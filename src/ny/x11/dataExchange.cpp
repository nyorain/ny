// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/dataExchange.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
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

//small DefaultAsyncRequest derivate that will unregister itself from pending requests
//on destruction
template<typename T>
class X11AsyncRequestImpl : public DefaultAsyncRequest<T>
{
public:
	using DefaultAsyncRequest<T>::DefaultAsyncRequest;
	nytl::ConnectionGuard connection;
};

//X11AsyncRequest derivate that will be used for data requests that first have to wait
//for the supported formats request to complete
class X11DataFormatRequestImpl : public X11AsyncRequestImpl<std::any>
{
public:
	using X11AsyncRequestImpl<std::any>::X11AsyncRequestImpl;
	DataOffer::FormatsRequest formatsRequest;
};

//DataOffer
X11DataOffer::X11DataOffer(X11AppContext& ac, unsigned int selection, xcb_window_t owner)
	: appContext_(&ac), selection_(selection), owner_(owner)
{
	//ask the selection owner to enumerate targets
	xcb_convert_selection(&appContext().xConnection(), appContext().xDummyWindow(), selection_,
		appContext().atoms().targets, appContext().atoms().clipboard, XCB_CURRENT_TIME);
}

X11DataOffer::~X11DataOffer()
{
	//signal all pending format requests that they have failed
	pendingFormatRequests_({});

	//singla all pending data requests that they have failed
	for(auto& pdr : pendingDataRequests_) pdr.second({});
}

X11DataOffer::FormatsRequest X11DataOffer::formats()
{
	// using RequestImpl = DefaultAsyncRequest<std::vector<DataFormat>>;
	using RequestImpl = X11AsyncRequestImpl<std::vector<DataFormat>>;

	//check if formats have already been retrieved
	if(formatsRetrieved_)
	{
		std::vector<DataFormat> fmts {};
		fmts.reserve(formats_.size());
		for(auto& fmt : formats_) fmts.push_back(fmt.first);

		return std::make_unique<RequestImpl>(std::move(fmts));
	}

	//the returned request will unregister the callback automatically on destruction,
	//therefore we can capute it and use it inside the callback.
	//the request will additionally be completed (with failure) when this DataOffer is
	//destructed
	auto ret = std::make_unique<RequestImpl>(appContext());
	auto fmtCallback = [req = ret.get()](nytl::ConnectionRef conn, std::vector<DataFormat> fmts) {
		conn.disconnect();
		req->complete(std::move(fmts));
	};

	ret->connection = pendingFormatRequests_.add(fmtCallback);
	return ret;
}

X11DataOffer::DataRequest X11DataOffer::data(const DataFormat& fmt)
{
	//if we did not even retrieved the supported formats we actually have to create
	//an extra request for the format
	if(!formatsRetrieved_)
	{
		auto ret = std::make_unique<X11DataFormatRequestImpl>(appContext());
		ret->formatsRequest = formats();

		//the returned request will unregister the callback on destruction and when
		//this DataOffer is destroyed, therefore both can be accessed (both are not movable)
		//this callback is triggered when the formats are retrieved and it will (try to) request
		//data in the request format.
		ret->formatsRequest->callback([fmt, this, req = ret.get()]{
			auto connection = this->registerDataRequest(fmt, *req);
			if(connection.connected()) req->connection = std::move(connection);
		});

		return ret;
	}

	//Just return a default data request that is waiting for the data to arrive
	auto ret = std::make_unique<X11AsyncRequestImpl<std::any>>(appContext());
	ret->connection = registerDataRequest(fmt, *ret);
	if(ret->connection.connected()) return {};
	return ret;
}

nytl::ConnectionGuard X11DataOffer::registerDataRequest(const DataFormat& format,
	DefaultAsyncRequest<std::any>& request)
{
	//check if the requested format is supported at all and query the associated
	//target format atom
	xcb_atom_t target = 0u;
	for(auto& mapping : formats_)
	{
		if(mapping.first == format)
		{
			target = mapping.second;
			break;
		}
	}

	//if the format is not supported complete the request with an empty data object and return
	//an empty connection
	if(!target)
	{
		request.complete({});
		return {};
	}

	//request the data from the selection owner that offers the data

	//add a callback to the pending data callbacks that will be triggered as soon
	//as we receive the data in the requested format
	//this callback will unregister itself.
	return pendingDataRequests_[target].add(
		[req = &request](nytl::ConnectionRef conn, const std::any& any) {
			req->complete(any);
			conn.disconnect();
	});
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
		}
	}
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

std::vector<xcb_atom_t> formatToTargetAtom(const X11AppContext& ac, const DataFormat& format)
{
	auto& atoms = ac.atoms();

	std::vector<xcb_atom_t> textAtoms {
		atoms.utf8string,
		atoms.mime.textPlainUtf8,
		XCB_ATOM_STRING,
		atoms.text,
		atoms.mime.textPlain
	};

	std::vector<xcb_atom_t> uriListAtoms {

	};

	if(format == DataFormat::text) return textAtoms;
	if(format == Dataformat::uriList)
	{
		textAtoms.insert(textAtoms.begin(), atoms.fileName);
		textAtoms.insert(textAtoms.begin(), atoms.mime.textUriList);
		return textAtoms;
	}

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

DataFormat targetAtomToFormat(const X11AppContext& ac, xcb_atom_t atom)
{
	auto& atoms = ac.atoms();

	if(atom == atoms.utf8string) return DataFormat::text;
	if(atom == XCB_ATOM_STRING) return DataFormat::text;
	if(atom == atoms.text) return DataFormat::text;
	if(atom == atoms.mime.textPlain) return DataFormat::text;
	if(atom == atoms.mime.textPlainUtf8) return DataFormat::text;
	if(atom == atoms.fileName) return DataFormat::uriList;
	if(atom == atoms.mime.imageData) return DataFormat::image;
	if(atom == atoms.mime.raw) return DataFormat::raw;

	auto nameCookie = xcb_get_atom_name(&ac.xConnection(), atom);

	xcb_generic_error_t* error {};
	auto nameReply = xcb_get_atom_name_reply(&ac.xConnection(), nameCookie, &error);

	if(error)
	{
		auto msg = x11::errorMessage(ac.xDisplay(), error->error_code);
		warning("ny::x11::targetAtomToFormat: get_atom_name_reply: ", msg);
		free(error);
		return DataFormat::none;
	}
	else if(!nameReply)
	{
		warning("ny::x11::targetAtomToFormat: get_atom_name_reply failed without error");
		return DataFormat::none;
	}

	auto name = xcb_get_atom_name_name(nameReply);
	auto string = std::string(name, name + xcb_get_atom_name_name_length(nameReply));
	free(nameReply);
	return {std::move(string)};
}


}
