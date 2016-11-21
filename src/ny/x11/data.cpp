// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/data.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/log.hpp>
#include <algorithm>

namespace ny
{

// the data manager was modeled after the clipboard specification of iccccm
// https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#use_of_selection_atoms

//TODO: something about notify event timeout
// probably best done using a timerfd and fd callback support in x11appcontext
//TODO: timestamps currently not implemented like in icccm convention
//TODO: should we surrender owned selections on X11DataManager destruction?

//DataOffer
X11DataOffer::X11DataOffer(X11AppContext& ac, unsigned int selection, xcb_window_t owner)
	: appContext_(&ac), selection_(selection), owner_(owner)
{
	//ask the selection owner to enumerate targets
	xcb_convert_selection(&appContext().xConnection(), appContext().xDummyWindow(), selection_,
		appContext().atoms().targets, appContext().atoms().clipboard, XCB_CURRENT_TIME);
}

void X11DataOffer::notify(const xcb_selection_notify_event_t& notify)
{
	auto& atoms = appContext().atoms();

	xcb_generic_error_t error;
	auto prop = x11::readProperty(appContext().xConnection(), atoms.clipboard,
		appContext().xDummyWindow(), &error);

	//check the target of the notify
	//if the target it atom.target, it notifies us that the selection owner set the
	//property of the dummy window to the list of supported (convertable targets)
	//otherwise it sets it to the data of some specific requested target
	if(notify.target == appContext().atoms().targets)
	{
		if(prop.format != 32 || prop.data.empty())
		{
			log("ny::X11DataOffer::notify: received invalid targets notify");
			return;
		}

		auto& targetAtoms = reinterpret_cast<const xcb_atom_t&>(*prop.data.data());
		for(auto atom : nytl::Range<xcb_atom_t>(targetAtoms, prop.data.size() / 4))
		{
			auto format = targetAtomToFormat(atoms, atom);
			if(format != dataType::none) dataTypes_.push_back({format, atom});
		}
	}
	else
	{
		auto target = notify.target;
		auto it = requests_.find(target);
		if(it == requests_.end())
		{
			log("ny::X11DataOffer::notify: received notify with unkown/not requested target");
			return;
		}

		//find the associated dataType format
		auto format = 0u;
		for(auto& dt : dataTypes_)
		{
			if(dt.second == target)
			{
				format = dt.first;
				break;
			}
		}

		//construct an associated object for the proerty data
		std::any any;

		if(format == dataType::raw)
		{
			any = std::move(prop.data);
		}
		else if(format == dataType::text)
		{
			std::string string = {prop.data.begin(), prop.data.end()};
			any = string;
		}
		else if(format == dataType::uriList && target == atoms.fileName)
		{
			std::vector<std::string> files = {{prop.data.begin(), prop.data.end()}};
			any = files;
		}
		else if(format == dataType::uriList && target == atoms.mime.textUriList)
		{
			std::string escaped = {prop.data.begin(), prop.data.end()};
			std::vector<std::string> files;

			//first create a copy of the string in which all escape codes (e.g. %20) are
			//replaced.
			std::string uris;
			uris.reserve(escaped.size());
			for(auto i = 0u; i < escaped.size(); ++i)
			{
				if(escaped[i] != '%')
				{
					uris.insert(uris.end(), escaped[i]);
					continue;
				}

				if(i + 2 >= escaped.size()) break;

				char number[3] = {escaped[i + 1], escaped[i + 2], 0};
				auto num = std::strtol(number, nullptr, 16);

				if(!num) log("ny::X11DataOffer: invalid escaped uri list code ", number);
				else uris.insert(uris.end(), num);

				i += 2;
			}

			//then split the uri list in its seperate uris and insert them
			//into the vector
			//also check for comment lines
			auto beg = 0;
			auto end = 0;

			do
			{
				beg = end;
				end = uris.find("\r\n", beg);
				if(uris[beg] != '#') files.push_back(uris.substr(beg, end));
			} while(end != std::string::npos);

			any = std::move(files);
		}

		it->second(any, *this, format);
		requests_.erase(format);
	}
}

X11DataOffer::FormatsRequest X11DataOffer::formats() const
{

}

X11DataOffer::DataRequest X11DataOffer::data(unsigned int fmt, const DataFunction& func)
{
	xcb_atom_t target = 0u;
	for(auto& mapping : dataTypes_)
	{
		if(mapping.first == fmt)
		{
			target = mapping.second;
			break;
		}
	}

	if(!target)
	{
		warning("ny::X11DataOffer: unsupported format given");
		func({}, *this, fmt);
		return {};
	}

	return requests_[target].add(func);
}

//X11DataManager
X11DataManager::X11DataManager(X11AppContext& ac) : appContext_(&ac)
{
}

bool X11DataManager::handleEvent(xcb_generic_event_t& ev)
{
	auto responseType = ev.response_type & ~0x80;
    switch(responseType)
    {
		case XCB_SELECTION_NOTIFY:
		{
			//a selectin owner notifies us that it set the request property
			//in the dummyWindow
			auto& notify = reinterpret_cast<xcb_selection_notify_event_t&>(ev);

			if(notify.requestor != appContext().xDummyWindow())
			{
				log("ny::X11DataManager: received selection notify for invalid window");
				return true;
			}

			X11DataOffer* dataOffer {};
			if(notify.selection == appContext().atoms().clipboard) dataOffer = &clipboardOffer_;
			else if(notify.selection == XCB_ATOM_PRIMARY) dataOffer = &primaryOffer_;
			else if(notify.selection == appContext().atoms().xdndSelection)
			{
			}

			if(dataOffer) dataOffer->notify(notify);
			else log("ny::X11DataManager: received selection notify for invalid selection");

			return true;
		}

		case XCB_SELECTION_REQUEST:
		{
			//some application asks us to convert a selection
			auto& req = reinterpret_cast<xcb_selection_request_event_t&>(ev);

			// if(req.owner != xDummyWindow())

			DataSource* source = nullptr;
			if(req.selection == atoms().clipboard) source = clipboardSource_.get();
			else if(req.selection == XCB_ATOM_PRIMARY) source = primarySource_.get();
			else if(req.selection == atoms().xdndSelection) source = dndSource_.get();

			if(source) answerRequest(*source, req);
			else
			{
				log("ny::X11DataManager: received unknown selection request");

				xcb_selection_notify_event_t notifyEvent {};
				notifyEvent.response_type = XCB_SELECTION_NOTIFY;
				notifyEvent.selection = req.selection;
				notifyEvent.property = XCB_NONE;
				notifyEvent.target = req.target;
				notifyEvent.requestor = req.requestor;

				auto eventData = reinterpret_cast<const char*>(&notifyEvent);
				xcb_send_event(&xConnection(), 0, req.requestor, 0, eventData);
			}

			return true;
		}

		case XCB_SELECTION_CLEAR:
		{
			//we are notified that we lost some selection ownership
			auto& clear = reinterpret_cast<xcb_selection_clear_event_t&>(ev);
			if(clear.owner != xDummyWindow())
			{
				log("ny::X11DataManager: received selection clear event for invalid window");
				return true;
			}

			if(clear.selection == atoms().clipboard) clipboardSource_.reset();
			else if(clear.selection == XCB_ATOM_PRIMARY) primarySource_.reset();

			return true;
		}

		case XCB_CLIENT_MESSAGE:
		{
			auto& clientm = reinterpret_cast<xcb_client_message_event_t&>(ev);
			if(clientm.type == appContext().atoms().xdndEnter)
			{
				auto* data = clientm.data.data32;

				bool typeList = data[1] & 1;
				xcb_window_t source = data[0];
				auto protocolVersion = data[1] >> 24;

				if(typeList)
				{
				}
				else
				{
				}
			}
			else if(clientm.type == appContext().atoms().xdndPosition)
			{
				//reply with dnd status message
			}
			else if(clientm.type == appContext().atoms().xdndLeave)
			{
				//reset the currently active data offer
				currentDndOffer_ = {};
			}
			else if(clientm.type == appContext().atoms().xdndDrop)
			{
				//generate a data offer event
				//push the current data offer into the vector vector with old ones
				auto wc = appContext().windowContext(clientm.window);
				if(wc && wc->eventHandler())
				{
					DataOfferEvent event(wc->eventHandler());
					event.offer = std::make_unique<X11DataOffer>(std::move(currentDndOffer_));
					wc->eventHandler()->handleEvent(event);
				}
			}
			else
			{
				return false;
			}

			return true;
		}

		default: return false;
	}
}

bool X11DataManager::clipboard(std::unique_ptr<DataSource>&& dataSource)
{
	//try to get ownership of selection
    xcb_set_selection_owner(&xConnection(), xDummyWindow(), atoms().clipboard, XCB_CURRENT_TIME);

	//check for success (-> ICCCM)
	auto owner = selectionOwner(atoms().clipboard);
	if(owner != xDummyWindow()) return false;

    clipboardSource_ = std::move(dataSource);
	return true;
}

DataOffer* X11DataManager::clipboard()
{
	auto owner = selectionOwner(atoms().clipboard);

	if(!owner) return nullptr;
	else if(owner != clipboardOffer_.owner())
		clipboardOffer_ = {appContext(), atoms().clipboard, owner};

	return &clipboardOffer_;
}

xcb_window_t X11DataManager::selectionOwner(xcb_atom_t selection)
{
	xcb_generic_error_t* error {};
	auto ownerCookie = xcb_get_selection_owner(&xConnection(), atoms().clipboard);
	auto reply = xcb_get_selection_owner_reply(&xConnection(), ownerCookie, &error);

	xcb_window_t owner {};
	if(reply)
	{
		owner = reply->owner;
		free(reply);
	}
	else if(error)
	{
		auto msg = x11::errorMessage(*appContext().xDisplay(), error->error_code);
		warning("ny::X11DataManager::selectionOwner: xcb_get_selection_owner: ", msg);
		free(error);
	}

	return owner;
}

void X11DataManager::answerRequest(DataSource& source,
	const xcb_selection_request_event_t& request)
{
	auto property = request.property;
	if(!property) property = request.target;

	auto dataTypes = source.types();
	if(request.target == appContext().atoms().targets)
	{
		//store a list with all supported formats converted to atoms
		std::vector<uint32_t> targets;
		targets.reserve(dataTypes.types.size());

		for(auto type : dataTypes.types)
		{
			auto atoms = formatToTargetAtom(appContext().atoms(), type);
			if(!atoms.empty()) targets.insert(targets.end(), atoms.begin(), atoms.end());
		}

		//remove duplicates
		std::sort(targets.begin(), targets.end());
		targets.erase(std::unique(targets.begin(), targets.end()), targets.end());

		xcb_change_property(&xConnection(), XCB_PROP_MODE_REPLACE, request.requestor,
			property, XCB_ATOM_ATOM, 32, targets.size(), targets.data());
	}
	else
	{
		//let the source convert the data to the request type and send it (store as property)
		auto fmt = targetAtomToFormat(appContext().atoms(), request.target);
		if(!fmt || !dataTypes.contains(fmt)) property = 0u;
		else
		{
			auto any = source.data(fmt);

			//convert the value of the any to a raw buffer in some way
			auto format = 0u;
			auto size = 0u;
			auto type = XCB_ATOM_ANY;
			void* data = {};
			std::vector<uint8_t> ownedBuffer;

			if(fmt == dataType::text)
			{

			}
			else if(fmt == dataType::uriList)
			{

			}
			else if(fmt == dataType::image)
			{

			}
			else if(fmt == dataType::timePoint)
			{

			}
			else if(fmt == dataType::timeDuration)
			{

			}

			xcb_change_property(&xConnection(), XCB_PROP_MODE_REPLACE, request.requestor,
				property, type, format, size, data);
		}
	}

	//notify the requestor
	xcb_selection_notify_event_t notifyEvent {};
	notifyEvent.selection = request.selection;
	notifyEvent.property = property;
	notifyEvent.target = request.target;
	notifyEvent.requestor = request.requestor;

	auto eventData = reinterpret_cast<const char*>(&notifyEvent);
	xcb_send_event(&xConnection(), 0, request.requestor, 0, eventData);
}

std::vector<xcb_atom_t> formatToTargetAtom(const x11::Atoms& atoms, unsigned int format)
{
	std::vector<xcb_atom_t> textAtoms {atoms.utf8string, atoms.mime.textPlainUtf8,
		XCB_ATOM_STRING, atoms.text, atoms.mime.textPlain};

	switch(format)
	{
		case dataType::raw: return {atoms.mime.raw};
		case dataType::text: return textAtoms;
		case dataType::uriList:
			textAtoms.insert(textAtoms.begin(), atoms.fileName);
			textAtoms.insert(textAtoms.begin(), atoms.mime.textUriList);
			return textAtoms;

		case dataType::image: return {atoms.mime.imageData, atoms.mime.imageBmp};
		case dataType::timePoint: return {atoms.mime.timePoint};
		case dataType::timeDuration: return {atoms.mime.timeDuration};
		case dataType::bmp: return {atoms.mime.imageBmp};
		case dataType::png: return {atoms.mime.imagePng};
		case dataType::jpeg: return {atoms.mime.imageJpeg};
		case dataType::gif: return {atoms.mime.imageGif};
		default: return {};
	}
}

unsigned int targetAtomToFormat(const x11::Atoms& atoms, xcb_atom_t atom)
{
	if(atom == atoms.utf8string) return dataType::text;
	if(atom == XCB_ATOM_STRING) return dataType::text;
	if(atom == atoms.text) return dataType::text;
	if(atom == atoms.fileName) return dataType::uriList;
	if(atom == atoms.mime.textPlain) return dataType::text;
	if(atom == atoms.mime.textPlainUtf8) return dataType::text;

	if(atom == atoms.mime.imageJpeg) return dataType::jpeg;
	if(atom == atoms.mime.imageGif) return dataType::gif;
	if(atom == atoms.mime.imagePng) return dataType::png;
	if(atom == atoms.mime.imageBmp) return dataType::bmp;

	if(atom == atoms.mime.imageData) return dataType::image;
	if(atom == atoms.mime.timePoint) return dataType::timePoint;
	if(atom == atoms.mime.timeDuration) return dataType::timeDuration;
	if(atom == atoms.mime.textUriList) return dataType::uriList;
	if(atom == atoms.mime.raw) return dataType::raw;

	return dataType::none;
}


}
