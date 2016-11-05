#include <ny/x11/data.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/log.hpp>

namespace ny
{

// the data manager was modeled after the clipboard specification of iccccm
// https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#use_of_selection_atoms

// additional resources:
// https://www.irif.fr/~jch/software/UTF8_STRING/UTF8_STRING.text
// https://www.freedesktop.org/wiki/Specifications/XDND/

//DataOffer
void X11DataOffer::notify(const xcb_selection_notify_event_t& notify)
{
	auto propCookie = xcb_get_property(&xConnection(), true, dummyWindow(),
		appContext().atoms().clipboard, XCB_ATOM_ANY, 0, 32);
	auto reply = xcb_get_property_reply(&xConnection(), propCookie, nullptr);

	auto data = xcb_get_property_value(reply);
	auto length = xcb_get_property_value_length(reply);

	free(reply);
}

//X11DataManager
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
			X11DataOffer* dataOffer;

			if(notify.selection == appContext().atoms().clipboard) dataOffer = &clipboardOffer_;
			else if(notify.selection == appContext().atoms().primary) dataOffer = &primaryOffer_;
			else if(notify.selection == appContext().atoms().xdndSelection)
			{
			}
			else
			{
				log("ny::X11DataManager::handleEvent: received unknown selection notify");
			}

			if(dataOffer) dataOffer->notify(notify);
			else log("ny::X11DataManager::handleEvent: received unknown selection notify");
			
			return true;
		}

		case XCB_SELECTION_REQUEST:
		{
			//some application asks us to convert a selection
			auto& req = reinterpret_cast<xcb_selection_request_event_t&>(ev);

			DataSource* source = nullptr;
			if(req.selection == appContext().atoms().clipboard) source = clipboardSource_.get();
			else if(req.selection == appContext().atoms().primary) source = primarySource_.get();
			else if(req.selection == appContext().atoms().xdndSelection) source = dndSource_.get();

			if(source) answerRequest(*source, req);
			else log("ny::X11DataManager::handleEvent: received unknown selection request");

			return true;
		}

		case XCB_SELECTION_CLEAR:
		{
			//we are notified that we lost some selection ownership
			return true;
		}

		case XCB_CLIENT_MESSAGE:
		{
			auto& clientm = reinterpret_cast<xcb_client_message_event_t&>(ev);
			if(clientm.type == dndEnterAtom_)
			{
			}
			else if(clientm.type == dndPositionAtom_)
			{
			}
			else if(clientm.type == dndLeaveAtom_)
			{
			}
			else if(clientm.type == dndDropAtom_)
			{
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

void X11DataManager::answerRequest(DataSource& source, 
	const xcb_selection_request_event_t& request)
{
	auto property = request.property;
	if(!property) property = request.target;

	auto dataTypes = source.types();
	if(request.target == appContext()->atoms().targets)
	{
		//store a list with all supported formats converted to atoms
		std::vector<uint32_t> targets;
		targets.reserve(dataTypes.types.size());

		for(auto type : dataTypes.types)
		{
			auto atom = formatToTargetAtom(type);
			if(atom) targets.push_back(atom);
		}
	
		xcb_change_property(&xConnection(), XCB_PROP_MODE_REPLACE, request.requestor, 
			property, XCB_ATOM_ATOM, 32, targets.size(), targets.data());
	}
	else
	{
		//let the source convert the data to the request type and store it
		auto fmt = targetAtomToFormat(request.target);
		if(!fmt || !dataTypes.contains(fmt)) property = 0u;
		auto data = source.data(fmt);
	}

	xcb_selection_notify_event_t notifyEvent {};
	notifyEvent.selection = request.selection;
	notifyEvent.property = property;
	notifyEvent.target = request.target;

	auto eventData = reinterpret_cast<const char*>(&notifyEvent);
	xcb_send_event(&xConnection(), 0, request.requestor, 0, eventData);
}

std::vector<xcb_atom_t> X11DataManager::formatToTargetAtom(unsigned int format)
{
	auto& atoms = appContext().atoms();
	std::vector<xcb_atom_t> textAtoms {atoms.utf8string, atoms.mime.textPlainUtf8,
		atoms.string, atoms.text, atoms.mime.textPlain};

	switch(format)
	{
		case dataType::raw: return {atoms.mime.raw};
		case dataType::text: return textAtoms;
		case dataType::filePaths: textAtoms.insert(textAtoms.begin(), atoms.mime.textUriList); 
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

unsigned int X11DataManager::targetAtomToFormat(xcb_atom_t atom)
{
	auto& atoms = appContext().atoms();

	if(atom == atoms.utf8string) return dataType::text;
	if(atom == atoms.string) return dataType::text;
	if(atom == atoms.text) return dataType::text;
	if(atom == atoms.mime.textPlain) return dataType::text;
	if(atom == atoms.mime.textPlainUtf8) return dataType::text;

	if(atom == atoms.mime.imageJpeg) return dataType::jpeg;
	if(atom == atoms.mime.imageGif) return dataType::gif;
	if(atom == atoms.mime.imagePng) return dataType::png;
	if(atom == atoms.mime.imageBmp) return dataType::bmp;

	if(atom == atoms.mime.imageData) return dataType::image;
	if(atom == atoms.mime.timePoint) return dataType::timePoint;
	if(atom == atoms.mime.timeDuration) return dataType::timeDuration;
	if(atom == atoms.mime.raw) return dataType::raw;

	return dataType::none;
}


}
