// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/dataExchange.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/input.hpp>
#include <ny/x11/bufferSurface.hpp>
#include <ny/common/copy.hpp>
#include <ny/bufferSurface.hpp>
#include <dlg/dlg.hpp>
#include <nytl/vecOps.hpp>

#include <X11/Xcursor/Xcursor.h>
#ifdef GenericEvent // macro defined by some X11 header...
	#undef GenericEvent
#endif

#include <algorithm>

// the data manager was modeled after the clipboard specification of iccccm
// https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#use_of_selection_atoms
// https://freedesktop.org/wiki/Specifications/XDND/
//
// simple (and old) dnd implementation:
// https://git.blender.org/gitweb/gitweb.cgi/blender.git/blob/HEAD:/extern/xdnd/xdnd.c

// TODO: something about notify event timeout (or leave it to application?)
//   probably best done using a timerfd and fd callback support in x11appcontext
// TODO: icccm not implemented correctly in all parts. Especially
//   care for special targets (XCB_TIMESTAMP retrievel, MULTIPLE, INCR,
//   DELETE, INSERT_SELECTION etc)
// TODO: some issues with xdnd corner cases. Make sure all version of
//   the protocol work correctly (read all spec version changes and make
//   sure we honor everything)
// TODO: xdndproxy not supported at the moment

namespace ny {
namespace {

constexpr auto dndGrabEventMask =
	XCB_EVENT_MASK_BUTTON_RELEASE |
	XCB_EVENT_MASK_ENTER_WINDOW |
	XCB_EVENT_MASK_LEAVE_WINDOW |
	XCB_EVENT_MASK_POINTER_MOTION;


/// Checks if the given window is dnd aware. Returns the xdnd protocol version
/// that should be used to communicate with the window or 0 if none is supported.
unsigned int xdndAware(X11AppContext& ac, xcb_window_t window) {
	static constexpr uint32_t supportedXdndVersion = 5; // we support xdnd version 5
	xcb_generic_error_t error {};

	//read the xdndAware property
	auto prop = x11::readProperty(ac.xConnection(), ac.atoms().xdndAware, window, &error);
	if(error.error_code || prop.data.size() < 4 || prop.type != XCB_ATOM_ATOM) {
		if(error.error_code) {
			auto msg = x11::errorMessage(ac.xDisplay(), error.error_code);
			dlg_warn("x11::readProperty: {}", msg);
			return 0u;
		}

		return 0u;
	}

	auto protocolVersion = reinterpret_cast<uint32_t&>(*prop.data.data());
	return std::min(protocolVersion, supportedXdndVersion);
}

} // anonymous util namespace

// - Implementation
// DataOffer
X11DataOffer::X11DataOffer(X11AppContext& ac, unsigned int selection,
	xcb_window_t owner, xcb_timestamp_t time) :
		appContext_(&ac), selection_(selection), owner_(owner),
		lastTime_(time) {
	// ask the selection owner to enumerate targets, i.e. to send us
	// all supported format
	formatReq_.time = time;
	xcb_convert_selection(&appContext().xConnection(),
		appContext().xDummyWindow(), selection_,
		appContext().atoms().targets, appContext().atoms().clipboard,
		lastTime_);
}

X11DataOffer::X11DataOffer(X11AppContext& ac, unsigned int selection,
	xcb_window_t owner, nytl::Span<const xcb_atom_t> supportedTargets,
	unsigned xdndVersion, xcb_timestamp_t time) :
		appContext_(&ac), selection_(selection), owner_(owner),
		lastTime_(time), xdndVersion_(xdndVersion) {
	// just parse/add all supported target formats.
	// addFormats sets formatsRetrieved_ to true, so in formats() we can
	// directly return a completed FormatsRequest
	setFormats(supportedTargets);
}

X11DataOffer::~X11DataOffer() {
	// signal all pending format requests that format retrieval has failed
	for(auto& listener : formatReq_.listeners) {
		listener({});
	}

	// singal all pending data requests that data retrieval has failed
	for(auto& dataReq : dataReqs_) {
		for(auto& listener : dataReq.listeners) {
			listener({});
		}
	}

	// if the Application had ownership over this DataOffer we have to
	// unregister from the DataManager so no further selection notifty events
	// are dispatched to us
	if(finishDnd_) {
		appContext().dataManager().unregisterDataOffer(*this);

		// NOTE: we have no other way to know whether drop was succesful
		// maybe integrate that to api?
		xcb_atom_t actionAtom = XCB_ATOM_NONE;
		if(action() == DndAction::copy) {
			actionAtom = appContext().atoms().xdndActionCopy;
		} else if(action() == DndAction::move) {
			actionAtom = appContext().atoms().xdndActionMove;
		}

		// send xdndfinished
		xcb_client_message_event_t revent {};
		revent.response_type = XCB_CLIENT_MESSAGE;
		revent.window = owner();
		revent.type = appContext().atoms().xdndFinished;
		revent.format = 32u;
		revent.data.data32[0] = over_;
		revent.data.data32[1] = (actionAtom) & 1u;
		revent.data.data32[2] = actionAtom;

		auto ptr = reinterpret_cast<char*>(&revent);
		xcb_send_event(&appContext().xConnection(), 0, revent.window, 0, ptr);
		xcb_flush(&appContext().xConnection());
	}
}

bool X11DataOffer::formats(FormatsListener listener) {
	// check if formats have already been retrieved
	if(formatsRetrieved_) {
		std::vector<std::string> fmts {};
		fmts.reserve(formats_.size());
		for(auto& fmt : formats_) {
			fmts.push_back(fmt.first);
		}

		listener(fmts);
		return true;
	}

	dlg_error("waiting formats request");

	// formats are already requested in constructor
	formatReq_.listeners.push_back(std::move(listener));
	return true;
}

void X11DataOffer::preferred(nytl::StringParam format, DndAction action) {
	// copy action is always allows per spec
	if(action != DndAction::none &&
			action != DndAction::copy &&
			action != action_) {
		dlg_warn("unsupported action passed to 'preferred'");
		action = DndAction::none;
	}

	if(formats_.find(std::string(format)) == formats_.end()) {
		dlg_warn("unsupported format passed to 'preferred'");
		format = {};
	}

	bool accepted = !format.empty() && action != DndAction::none;
	xcb_atom_t actionAtom = XCB_ATOM_NONE;
	if(accepted && action == DndAction::copy) {
		actionAtom = appContext().atoms().xdndActionCopy;
	} else if(accepted && action == DndAction::move) {
		actionAtom = appContext().atoms().xdndActionMove;
	}

	// answer with the current state, i.e. whether the application can/will accept
	// the offer or not
	xcb_client_message_event_t revent {};
	revent.response_type = XCB_CLIENT_MESSAGE;
	revent.window = owner();
	revent.type = appContext().atoms().xdndStatus;
	revent.format = 32u;
	revent.data.data32[0] = over_;
	revent.data.data32[1] = accepted;
	revent.data.data32[2] = 0;
	revent.data.data32[3] = 0;

	if(xdndVersion_ >= 2) {
		revent.data.data32[4] = actionAtom;
	}

	auto reventPtr = reinterpret_cast<char*>(&revent);
	xcb_send_event(&appContext().xConnection(), 0, revent.window, 0, reventPtr);
	xcb_flush(&appContext().xConnection());
}

bool X11DataOffer::addDataListener(nytl::StringParam format,
		DataListener&& listener) {
	// check if there already is a pending request
	for(auto& req : dataReqs_) {
		if(req.format == format) {
			req.listeners.emplace_back(std::move(listener));
			return true;
		}
	}

	// new request
	xcb_atom_t target = 0u;
	for(auto& mapping : formats_) {
		if(mapping.first == format) {
			target = mapping.second;
			break;
		}
	}

	// if the format is not supported complete the request with an empty data
	// object and return an empty connection
	if(!target) {
		return false;
	}

	// request the data in the target format from the selection owner that
	// offers the data we request the owner to store it into the clipboard atom
	// property of the dummy window
	auto cookie = xcb_convert_selection_checked(&appContext().xConnection(),
		appContext().xDummyWindow(), selection_, target,
		appContext().atoms().clipboard, lastTime_);
	appContext().errorCategory().checkWarn(cookie,
		"ny::X11DataOffer::addDataListener");

	auto& req = dataReqs_.emplace_back();
	req.time = lastTime_;
	req.target = target;
	req.format = format;
	req.listeners.push_back(std::move(listener));
	return true;
}

bool X11DataOffer::data(nytl::StringParam format, DataListener listener) {
	// if we haven't received the supported formats we can't know the mappings
	// e.g. if the request is for "text/plain" but the target supports
	// UTF8_TEXT we will use that, but if the target only supports TEXT,
	// we have to use that. We can't know in advance.
	if(!formatsRetrieved_) {
		auto sformat = std::string(format); // we need a copy of that
		auto formatListener = [this, sformat, dlistener = std::move(listener)]
			(nytl::Span<const std::string>) mutable {
				if(!addDataListener(sformat, std::move(dlistener))) {
					// listener wasn't moved in this case
					dlistener({});
				}
		};

		return formats(formatListener);
	}

	// otherwise we already have received the supported targets and
	// can directly add the data request
	return addDataListener(format, std::move(listener));
}

// NOTE: a malicious client can obviously destroy all of our logic
// but i'm not sure you can even implement these selection things
// in any solid way
bool X11DataOffer::notify(const xcb_selection_notify_event_t& notify) {
	// check the target of the notify
	// if the target it atoms.target, it notifies us that the selection owner set the
	// property of the dummy window to the list of supported (convertable targets)
	// otherwise it sets it to the data of some specific requested target
	if(notify.target == appContext().atoms().targets) {
		// check that timestamp is correct
		if(notify.time != formatReq_.time) {
			dlg_debug("invalid time: {} vs {}", notify.time, formatReq_.time);
			return false;
		}

		// no matter what happens, call pending format handlers
		std::vector<std::string> supportedFormats;
		auto completeListeners = nytl::ScopeGuard([&]{
			for(auto& listener : formatReq_.listeners) {
				listener(supportedFormats);
			}
			formatReq_ = {};
		});

		// formats were already received
		if(formatsRetrieved_) {
			dlg_warn("selection formats were already received");
			return true;
		}

		if(notify.property == 0) {
			dlg_info("TARGETS conversion failed at selection owner");
			return true;
		}

		xcb_generic_error_t err {};
		auto prop = x11::readProperty(appContext().xConnection(), notify.property,
			appContext().xDummyWindow(), &err);

		// check if error ocurred
		if(err.error_code) {
			auto msg = x11::errorMessage(appContext().xDisplay(), err.error_code);
			dlg_info("failed to read property: {}", msg);
			return true;
		}

		// the property must be set to a list of atoms which have 32 bit length
		// so if the format is not 32, the property/selectionEvent is invalid
		if(prop.format != 32 || prop.data.empty()) {
			dlg_info("received targets notify with invalid property data");
			return true;
		}

		// add the retrieved atoms the the list of supported targets/formats
		auto* targetAtoms = reinterpret_cast<const xcb_atom_t*>(prop.data.data());
		auto size = static_cast<unsigned int>(prop.data.size() / 4);
		setFormats({targetAtoms, size});

		// extract the supported data formats
		supportedFormats.reserve(formats_.size());
		for(auto& fmt : formats_) {
			supportedFormats.push_back(fmt.first);
		}
	} else {
		auto it = std::find_if(dataReqs_.begin(), dataReqs_.end(),
			[&](auto& req) { return req.target == notify.target; });
		if(it == dataReqs_.end()) {
			dlg_debug("invalid target");
			return false;
		}

		auto& req = *it;
		dlg_assert(!req.listeners.empty());

		if(req.time != notify.time) {
			dlg_debug("invalid time: {} vs {}", notify.time, formatReq_.time);
			return false;
		}

		// no matter what happens, call pending handlers
		ExchangeData data;
		auto completeListeners = nytl::ScopeGuard([&]{
			for(auto& listener : req.listeners) {
				listener(data);
			}
			dataReqs_.erase(it);
		});

		xcb_generic_error_t err {};
		auto prop = x11::readProperty(appContext().xConnection(), notify.property,
			appContext().xDummyWindow(), &err);

		if(err.error_code) {
			auto msg = x11::errorMessage(appContext().xDisplay(), err.error_code);
			dlg_info("failed to read property: {}", msg);
			return true;
		}

		// set the data to call the pending listeners with
		data = wrap(prop.data, req.format);
	}

	return true;
}

void X11DataOffer::setFormats(nytl::Span<const xcb_atom_t> targets) {
	// TODO: filter out special formats such as MULTIPLE or TIMESTAMP or stuff
	// they should not be advertised to the application

	std::vector<std::pair<xcb_get_atom_name_cookie_t, xcb_atom_t>> atomNameCookies;
	atomNameCookies.reserve(targets.size());

	auto& atoms = appContext().atoms();
	auto& xConn = appContext().xConnection();

	// check for known target atoms
	// if the target atom is not known, push a cookie to request its name
	for(auto& target : targets) {
		if(target == atoms.utf8string || target == atoms.mime.textPlainUtf8) {
			formats_.emplace(ny::mime::utf8, target);
		} else if(target == XCB_ATOM_STRING ||
				target == atoms.text ||
				target == atoms.mime.textPlain) {
			formats_.emplace(ny::mime::text, target);
		} else if(target == atoms.fileName || target == atoms.mime.textUriList) {
			formats_.emplace(ny::mime::uriList, target);
		} else if(target == atoms.mime.imageData) {
			formats_.emplace(ny::mime::image, target);
		} else if(target == atoms.mime.raw) {
			formats_.emplace(ny::mime::raw, target);
		} else {
			atomNameCookies.push_back({xcb_get_atom_name(&xConn, target), target});
		}
	}

	// try to get all requested names and insert them into formats_ if successful
	for(auto& cookie : atomNameCookies) {
		xcb_generic_error_t* err {};
		auto nameReply = xcb_get_atom_name_reply(&xConn, cookie.first, &err);

		if(err) {
			auto msg = x11::errorMessage(appContext().xDisplay(), err->error_code);
			dlg_warn("get_atom_name_reply failed: {}", msg);
			free(err);
			continue;
		} else if(!nameReply) {
			dlg_warn("get_atom_name_reply failed: without error");
			continue;
		}

		auto name = xcb_get_atom_name_name(nameReply);
		auto string = std::string(name,
			name + xcb_get_atom_name_name_length(nameReply));
		free(nameReply);
		formats_[std::move(string)] = cookie.second;
	}

	// remember that we have the formats retrieved, i.e. formats_ it complete now
	// so we don't request them again
	formatsRetrieved_ = true;
}

// X11DataSource
X11DataSource::X11DataSource(X11AppContext& ac, std::unique_ptr<DataSource> src,
	xcb_timestamp_t acquired) : appContext_(&ac), dataSource_(std::move(src)),
		acquired_(acquired) {

	// convert the supported formats of the source to target atoms
	auto& atoms = appContext().atoms();
	auto& xConn = appContext().xConnection();
	auto formats = dataSource_->formats();

	std::vector<std::pair<std::string, xcb_intern_atom_cookie_t>> atomCookies;
	atomCookies.reserve(formats.size());

	// check for known special formats
	for(auto& fmt : formats) {
		if(fmt == ny::mime::utf8) {
			formatsMap_.push_back({atoms.utf8string, fmt});
			formatsMap_.push_back({atoms.mime.textPlainUtf8, fmt});
			formatsMap_.push_back({atoms.text, fmt});
			formatsMap_.push_back({atoms.mime.textPlain, fmt});
			formatsMap_.push_back({XCB_ATOM_STRING, fmt});
		} else if(fmt == ny::mime::text) {
			formatsMap_.push_back({atoms.text, fmt});
			formatsMap_.push_back({atoms.mime.textPlain, fmt});
			formatsMap_.push_back({XCB_ATOM_STRING, fmt});
		} else if(fmt == ny::mime::uriList) {
			// TODO: when the file contains only one element we could
			// advertise to support the FILE_NAME target.
			// Not sure if this is expected behavior though
			formatsMap_.push_back({atoms.mime.textUriList, fmt});
		} else if(fmt == ny::mime::image) {
			formatsMap_.push_back({atoms.mime.imageData, fmt});
		} else if(fmt == ny::mime::raw) {
			formatsMap_.push_back({atoms.mime.raw, fmt});
		} else {
			auto atomCookie = xcb_intern_atom(&xConn, 0, fmt.size(), fmt.c_str());
			atomCookies.push_back({fmt, atomCookie});
		}
	}

	// receive all atoms for the supported formats and insert them into the
	// vector of supported formats
	for(auto& cookie : atomCookies) {
		xcb_generic_error_t* err {};
		auto reply = xcb_intern_atom_reply(&xConn, cookie.second, &err);
		if(reply) {
			formatsMap_.push_back({reply->atom, cookie.first});
			free(reply);
			continue;
		} else if(err) {
			auto msg = x11::errorMessage(appContext().xDisplay(), err->error_code);
			dlg_warn("Failed to load atom for {}", cookie.first);
			free(err);
		}
	}

	// extract a vector of supported targets since this is used pretty often
	// also remove duplicates from the targets vector
	targets_.reserve(formatsMap_.size());
	for(const auto& entry : formatsMap_) {
		targets_.push_back(entry.first);
	}

	std::sort(targets_.begin(), targets_.end());
	targets_.erase(std::unique(targets_.begin(), targets_.end()), targets_.end());
}

void X11DataSource::answerRequest(const xcb_selection_request_event_t& request) {
	// TODO: some parts of icccm not implemented
	auto property = request.property;
	if(!property) { // icccm specifies that it should be handled like this
		property = request.target;
	}

	if(request.target == appContext().atoms().targets) {
		// set the requested property of the requested window to the target atoms
		xcb_change_property(&appContext().xConnection(), XCB_PROP_MODE_REPLACE,
			request.requestor, property, XCB_ATOM_ATOM, 32,
			targets_.size(), targets_.data());
	} else {
		// let the source convert the data to the request type and send it (store as property)
		const char* format {};
		for(const auto& fmt : formatsMap_) {
			if(fmt.first == request.target) {
				format = fmt.second.c_str();
				break;
			}
		}

		if(!format) {
			dlg_warn("unsupported target request");
			property = 0u;
		} else {
			// request the data in the associated DataFormat from the source
			auto data = dataSource_->data(format);
			if(data.index() == 0u) { // invalid
				dlg_warn("data source could not provide data");
				property = 0u;
			} else {
				// convert the value of the any to a raw buffer in some way
				auto atomFormat = 8u;
				auto type = XCB_ATOM_STRING;
				auto buf = unwrap(data);

				// set the property of the requested window to the raw data
				xcb_change_property(&appContext().xConnection(),
					XCB_PROP_MODE_REPLACE, request.requestor, property, type,
					atomFormat, buf.size(), buf.data());
			}
		}
	}

	// notify the requestor
	xcb_selection_notify_event_t notifyEvent {};
	notifyEvent.response_type = XCB_SELECTION_NOTIFY;
	notifyEvent.selection = request.selection;
	notifyEvent.property = property;
	notifyEvent.target = request.target;
	notifyEvent.requestor = request.requestor;
	notifyEvent.time = request.time;

	auto eventPtr = reinterpret_cast<const char*>(&notifyEvent);
	xcb_send_event(&appContext().xConnection(), 0, request.requestor, 0, eventPtr);
	xcb_flush(&appContext().xConnection());
}

// X11DataManager
X11DataManager::X11DataManager(X11AppContext& ac) :
		appContext_(&ac) {

	X11WindowSettings settings;
	settings.transparent = true;
	settings.droppable = false;
	settings.show = false;
	settings.surface = SurfaceType::buffer;
	settings.size = {32, 32};

	settings.overrideRedirect = true;
	settings.windowType = ac.ewmhConnection()._NET_WM_WINDOW_TYPE_DND;
	dndSrc_.dndWindow = std::make_unique<X11BufferWindowContext>(ac, settings);
}

X11DataManager::~X11DataManager() {
	if(cursors_.dndCopy) {
		xcb_free_cursor(&xConnection(), cursors_.dndCopy);
	}
	if(cursors_.dndMove) {
		xcb_free_cursor(&xConnection(), cursors_.dndMove);
	}
	if(cursors_.dndNo) {
		xcb_free_cursor(&xConnection(), cursors_.dndNo);
	}

	// NOTE: we could surrender owned selections here. But per icccm
	// selection ownership will automatically return to None when
	// the owner window is destroyed (or the client terminated) so
	// it should be fine
}

void X11DataManager::initCursors() {
	// NOTE: for some reason, returns invalid cursors (that generate BadCursor
	// when used) when called in the constructor.
	// Therefore only called when needed later on
	if(cursors_.init) {
		return;
	}

	auto& xdpy = appContext().xDisplay();
	cursors_.dndCopy = XcursorLibraryLoadCursor(&xdpy, "dnd-copy");
	cursors_.dndMove = XcursorLibraryLoadCursor(&xdpy, "dnd-move");
	cursors_.dndNo = XcursorLibraryLoadCursor(&xdpy, "dnd-no-drop");
	cursors_.init = true;
}

bool X11DataManager::processEvent(const void* pev) {
	dlg_assert(pev);
	auto& ev = *static_cast<const xcb_generic_event_t*>(pev);
	X11EventData eventData(ev);

	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
	case XCB_SELECTION_NOTIFY: {
		// a selection owner notifies us that it set the request property
		// we need to query the DataOffer this is associated with and let it handle
		// the notification
		auto notify = copyu<xcb_selection_notify_event_t>(ev);
		auto& atoms = appContext().atoms();

		if(notify.requestor != appContext().xDummyWindow()) {
			dlg_info("received selection notify for invalid window");
			return true;
		}

		if(notify.selection == atoms.clipboard) {
			dlg_assertlm(dlg_level_info, clipboardOffer_->notify(notify),
				"received invalid clipboard selection notify event");
		} else if(notify.selection == XCB_ATOM_PRIMARY) {
			dlg_assertlm(dlg_level_info, primaryOffer_->notify(notify),
				"received invalid primary selection notify event");
		} else if(notify.selection == atoms.xdndSelection) {
			// try current one
			if(!dndOffer_.offer || !dndOffer_.offer->notify(notify)) {
				// if event wasn't for current one, try old ones
				bool found = false;
				for(auto& offer : oldDndOffers_) {
					if(offer->notify(notify)) {
						found = true;
						break;
					}
				}

				dlg_assertlm(dlg_level_info, found,
					"received invalid xdnd selection notify event");
			}
		}

		return true;
	} case XCB_SELECTION_REQUEST: {
		// some other x clients requests us to convert a selection or send
		// them the supported targets
		// find the associated data source and let it answer the request
		auto req = copyu<xcb_selection_request_event_t>(ev);

		X11DataSource* source = nullptr;
		if(req.selection == atoms().clipboard) {
			source = &clipboardSource_;
		} else if(req.selection == XCB_ATOM_PRIMARY) {
			source = &primarySource_;
		} else if(req.selection == atoms().xdndSelection) {
			source = &dndSrc_.source;
		}

		if(source) {
			source->answerRequest(req);
		} else {
			// if the request cannot be handled, simply send an event to the
			// client that notifies them that the request failed.
			dlg_info("received selection request for invalid selection");

			xcb_selection_notify_event_t notifyEvent {};
			notifyEvent.response_type = XCB_SELECTION_NOTIFY;
			notifyEvent.selection = req.selection;
			notifyEvent.property = XCB_NONE;
			notifyEvent.target = req.target;
			notifyEvent.requestor = req.requestor;

			auto eventPtr = reinterpret_cast<const char*>(&notifyEvent);
			xcb_send_event(&xConnection(), 0, req.requestor, 0, eventPtr);
			xcb_flush(&appContext().xConnection());
		}

		return true;
	} case XCB_SELECTION_CLEAR: {
		// we are notified that we lost ownership over a selection
		// query the associated data source and unset it
		auto clear = copyu<xcb_selection_clear_event_t>(ev);
		if(clear.owner != xDummyWindow()) {
			dlg_info("received selection clear event for invalid window");
			return true;
		}

		if(clear.selection == atoms().clipboard) {
			clipboardSource_ = {};
		} else if(clear.selection == XCB_ATOM_PRIMARY) {
			primarySource_ = {};
		}

		return true;
	} case XCB_CLIENT_MESSAGE: {
		// xdnd events are sent as client messages
		auto clientm = copyu<xcb_client_message_event_t>(ev);
		if(processClientMessage(clientm, eventData)) {
			return true;
		}
	} default: {
		break;
	}
	}

	// if we are currently grabbing the pointer because we initiated a dnd session
	// also handle the pointer events and forward them to dnd handlers.
	if(dndSrc_.sourceWindow && processDndEvent(pev)) {
		return true;
	}

	// we did never handle the event in any way
	return false;
}

bool X11DataManager::processClientMessage(
		const xcb_client_message_event_t& clientm,
		const EventData& eventData) {
	if(clientm.type == atoms().xdndEnter) {
		auto* data = clientm.data.data32;

		bool moreThan3 = data[1] & 1; // whether the supported targets are attached
		xcb_window_t source = data[0]; // the source of the dnd operation
		auto protocolVersion = data[1] >> 24; // the supported protocol version

		std::vector<xcb_atom_t> targets;
		targets.reserve(10);

		// extract the supported targets
		// if there are more than 3 we have to check xdndtypelist which
		// contains *all* targets (i.e. we can ignore data[2,3,4] then)
		if(!moreThan3) {
			// not more than 3 targets: they were sent directly in the event
			if(data[2] != XCB_ATOM_NONE) {
				targets.push_back(data[2]);
			}
			if(data[3] != XCB_ATOM_NONE) {
				targets.push_back(data[3]);
			}
			if(data[4] != XCB_ATOM_NONE) {
				targets.push_back(data[4]);
			}
		} else {
			// read the xdndtypelist atom property of the source window
			// for the supported targets
			xcb_generic_error_t err {};
			auto typeList = x11::readProperty(xConnection(),
				atoms().xdndTypeList, source, &err);

			if(err.error_code) {
				auto msg = x11::errorMessage(appContext().xDisplay(), err.error_code);
				dlg_warn("reading xdndTypeList property failed: {}", msg);
				return true;
			}

			auto begin = reinterpret_cast<xcb_atom_t*>(typeList.data.data());
			auto end = begin + (typeList.data.size() / 4);
			targets.insert(targets.end(), begin, end);
		}

		// getting the associated WindowContext
		auto windowContext = appContext().windowContext(clientm.window);
		if(!windowContext) {
			dlg_warn("xdndEnter for invalid x window");
			return true;
		}

		// create a new data offer and store it as current one
		// also store the protocol version used for the current session
		auto offer = std::make_unique<X11DataOffer>(appContext(),
			atoms().xdndSelection, source, targets, protocolVersion,
			appContext().time());
		offer->over(clientm.window);

		dndOffer_.offer = std::move(offer);
		dndOffer_.windowContext = windowContext;

		// send enter event to the listener of the associated window
		// we cannot send move event or set the position since we don't get this
		// information
		DndEnterEvent dee;
		dee.eventData = &eventData;
		dee.offer = dndOffer_.offer.get();
		windowContext->listener().dndEnter(dee);
	} else if(clientm.type == atoms().xdndPosition) {
		if(!dndOffer_.windowContext || !dndOffer_.offer) {
			dlg_info("xdndPosition event without current xdnd session");
			return true;
		}

		dndOffer_.offer->timestamp(clientm.data.data32[3]);
		if(dndOffer_.offer->xdndVersion() >= 2) {
			auto actionAtom = clientm.data.data32[4];
			auto action = DndAction::none;
			if(actionAtom == atoms().xdndActionCopy) {
				action = DndAction::copy;
			} else if(actionAtom == atoms().xdndActionMove) {
				action = DndAction::move;
			}
			dndOffer_.offer->action(action);
		}

		dndOffer_.offer->over(clientm.window);
		dndOffer_.offer->owner(clientm.data.data32[0]);

		// the position is given relative to the root window so we have to
		// translate it first
		auto rootwin = appContext().xDefaultScreen().root;
		auto cookie = xcb_translate_coordinates(&xConnection(), rootwin, clientm.window,
			clientm.data.data32[2] >> 16, clientm.data.data32[2] & 0xffff);

		xcb_generic_error_t* err {};
		auto reply = xcb_translate_coordinates_reply(&xConnection(), cookie, &err);
		if(err) {
			auto msg = x11::errorMessage(appContext().xDisplay(), err->error_code);
			dlg_info("xdndPosition: xcb_translate_coordinates: {}", msg);
			free(err);
			return true;
		}

		nytl::Vec2i pos{reply->dst_x, reply->dst_y};
		free(reply);

		DndMoveEvent dme;
		dme.eventData = &eventData;
		dme.position = pos;
		dme.offer = dndOffer_.offer.get();
		dndOffer_.windowContext->listener().dndMove(dme);
	} else if(clientm.type == atoms().xdndLeave) {
		std::unique_ptr<X11DataOffer> offer = std::move(dndOffer_.offer);
		auto wc = dndOffer_.windowContext;
		dndOffer_ = {};
		if(!wc || !offer) {
			dlg_info("xdndLeave event without current xdnd session");
			return true;
		}

		DndLeaveEvent dle;
		dle.eventData = &eventData;
		dle.offer = offer.get();
		wc->listener().dndLeave(dle);
	} else if(clientm.type == atoms().xdndDrop) {
		std::unique_ptr<X11DataOffer> offer = std::move(dndOffer_.offer);
		auto wc = dndOffer_.windowContext;
		dndOffer_ = {};

		// generate a data offer event
		// push the current data offer into the vector vector with old ones
		if(!wc || !offer) {
			dlg_info("xdndDrop event without current xdnd session");
			return true;
		}

		nytl::Vec2i pos {};
		pos[0] = clientm.data.data32[2] >> 16;
		pos[1] = clientm.data.data32[2] & 0xffff;

		oldDndOffers_.push_back(offer.get());
		offer->finishDnd(); // unregisters on destruction

		DndDropEvent dde;
		dde.position = pos;
		dde.offer = std::move(offer);
		dde.eventData = &eventData;
		wc->listener().dndDrop(dde);
	} else if(clientm.type == atoms().xdndStatus) {
		// xdndStatus: we (as drag source) received a status message from
		// the drop target we are currently over
		if(!dndSrc_.dndWindow) {
			dlg_info("xdndStatus event without current xdnd session");
			return true;
		}

		// in this case we received the event from a window that isn't
		// the one we are currently over. Can happen e.g. when we recently
		// left a window, not an error.
		if(clientm.data.data32[0] != dndSrc_.targetWindow) {
			return true;
		}

		// if there is a pending position update, send it
		if(dndSrc_.sendPos) {
			if(dndSrc_.targetWindow) {
				auto [pos, time] = *dndSrc_.sendPos;
				xdndSendPosition(pos, time);
			}
			dndSrc_.sendPos = {};
		}

		// update cursor and action
		auto accepted = (clientm.data.data32[1] & 1u);
		auto actionAtom = clientm.data.data32[4];
		auto action = DndAction::none;
		auto cursor = cursors_.dndNo;
		if(dndSrc_.version >= 2) {
			if(actionAtom == atoms().xdndActionCopy) {
				action = DndAction::copy;
				cursor = cursors_.dndCopy;
			} else if(actionAtom == atoms().xdndActionMove) {
				action = DndAction::move;
				cursor = cursors_.dndMove;
			}
		} else if(accepted) {
			action = DndAction::copy;
				cursor = cursors_.dndCopy;
		}

		dndSrc_.source.dataSource().action(action);
		xcb_change_active_pointer_grab(&xConnection(), cursor,
				appContext().time(), dndGrabEventMask);
	} else if(clientm.type == atoms().xdndFinished) {
		// TODO: cache old dnd data sources and dispatch events to them
		// until we receive this event. See header: oldDndSources_
	} else {
		return false; // if we land here we could not handle the client message
	}

	return true; // we handled the client message
}

bool X11DataManager::processDndEvent(const void* pev) {
	dlg_assert(pev);
	auto& ev = *static_cast<const xcb_generic_event_t*>(pev);

	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
	case XCB_MOTION_NOTIFY: {
		auto motionEv = copyu<xcb_motion_notify_event_t>(ev);
		appContext().time(motionEv.time);

		// move the dnd window. The +1 offset is a hack so that we
		// not get the dnd window from xcb_query_pointer.
		// Properly doing this involves using xcb_query_tree and
		// querying geometry for every window/child on the hierachy.
		// when we support hotspots/dnd window offset we have to use
		// the complicated/slow way.
		uint32_t values2[2] = {
			(uint32_t) motionEv.root_x + 1,
			(uint32_t) motionEv.root_y + 1};
		xcb_configure_window(&xConnection(), dndSrc_.dndWindow->xWindow(),
			XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values2);

		// query the xdnd window we are currently over
		// this here is a real performance killer, so optimizations like
		// caching the size would probably be a good idea
		// we have to do it this complex since often not the window we are over but
		// one of its ancestors is xdnd aware
		auto child = motionEv.event;
		xcb_window_t toplevel {};
		unsigned int version {};
		while(true) {
			xcb_generic_error_t* error {};
			auto cookie = xcb_query_pointer(&xConnection(), child);
			auto reply = xcb_query_pointer_reply(&xConnection(), cookie, &error);

			if(error) {
				auto msg = x11::errorMessage(appContext().xDisplay(), error->error_code);
				dlg_warn("xcb_query_pointer failed: {}", msg);
				free(error);
				break;
			}

			auto replyChild = reply->child;
			free(reply);

			if(!replyChild) {
				break;
			}

			child = replyChild;
			if(!toplevel) {
				version = xdndAware(appContext(), child);
				if(version) {
					toplevel = child;
				}
			}
		}

		// send leave event to old one and enter to new one if valid if it
		// changed otherwise just send a default position event
		if(toplevel != dndSrc_.targetWindow) {
			if(dndSrc_.targetWindow) {
				xdndSendLeave();
			}

			dndSrc_.pendingStatus_ = false;
			dndSrc_.targetWindow = toplevel;
			dndSrc_.version = version;
			if(dndSrc_.targetWindow) {
				xdndSendEnter();
			}
		} else if(dndSrc_.targetWindow) {
			auto pos = nytl::Vec2i{motionEv.root_x, motionEv.root_y};
			if(!dndSrc_.pendingStatus_) {
				xdndSendPosition(pos, motionEv.time);
			} else {
				// set/update it to be send the next time we receive
				// xdndstatus
				dndSrc_.sendPos = {pos, motionEv.root_x};
			}
		}

		return true;
	} case XCB_BUTTON_RELEASE: {
		auto buttonEv = copyu<xcb_button_release_event_t>(ev);
		appContext().time(buttonEv.time);

		if(dndSrc_.targetWindow) {

			// if we are over an xdnd window, send a drop event
			xcb_client_message_event_t clientev {};
			clientev.window = dndSrc_.targetWindow;
			clientev.format = 32;
			clientev.response_type = XCB_CLIENT_MESSAGE;
			clientev.type = atoms().xdndDrop;
			clientev.data.data32[0] = dndSrc_.sourceWindow;
			if(dndSrc_.version >= 1) {
				clientev.data.data32[2] = buttonEv.time;
			}


			auto eventPtr = reinterpret_cast<const char*>(&clientev);
			xcb_send_event(&xConnection(), 0, dndSrc_.targetWindow, 0, eventPtr);
		}

		// end the dnd session, i.e. ungrab the pointer
		// send a xdnddrop event if we are over an accepting window
		xcb_ungrab_pointer(&xConnection(), buttonEv.time);

		// reset all session values
		// not yet reset the dnd source sine it might be needed further
		dndSrc_.dndWindow->hide();
		dndSrc_.version = {};
		dndSrc_.targetWindow = {};
		dndSrc_.sourceWindow = {};
		dndSrc_.pendingStatus_ = {};
		dndSrc_.sendPos = {};

		return true;
	} default: {
		break;
	}
	}

	return false;
}

bool X11DataManager::clipboard(std::unique_ptr<DataSource>&& dataSource) {
	// try to get ownership of selection
	auto time = appContext().time();
	xcb_set_selection_owner(&xConnection(), xDummyWindow(), atoms().clipboard,
		time);

	// check for success
	// icccm specifies that it may fail and should only be considered
	// succesful if the owner is really changed
	auto owner = selectionOwner(atoms().clipboard);
	if(owner != xDummyWindow()) {
		return false;
	}

	clipboardSource_ = {appContext(), std::move(dataSource), time};
	return true;
}

DataOffer* X11DataManager::clipboard() {
	auto owner = selectionOwner(atoms().clipboard);

	// if there is no owner, the clipboard has currently no value
	// we also (impliclty) reset the clipboard data offer then because it
	// should no longer be used.
	if(!owner) {
		clipboardOffer_ = {};
		return nullptr;
	}

	//refresh the clipboard offer if needed to do so.
	if(!clipboardOffer_ || owner != clipboardOffer_->owner()) {
		clipboardOffer_ = std::make_unique<X11DataOffer>(appContext(),
			atoms().clipboard, owner, appContext().time());
	}

	return clipboardOffer_.get();
}

bool X11DataManager::startDragDrop(const EventData* evdata,
		std::unique_ptr<DataSource> src) {
	initCursors();

	// check if currently over window
	if(!appContext().mouseContext()->over()) {
		dlg_warn("startDragDrop: not having mouse focus");
		return false;
	}

	auto time = appContext().time();
	auto ev = dynamic_cast<const X11EventData*>(evdata);
	if(evdata) {
		// TODO: we could refactor this to X11EventData::timestamp()
		auto rtype = ev->event.response_type & ~0x80;
		if(rtype == XCB_BUTTON_PRESS) {
			time = copyu<xcb_button_press_event_t>(ev->event).time;
		} else if(rtype == XCB_BUTTON_RELEASE) {
			time = copyu<xcb_button_release_event_t>(ev->event).time;
		} else if(rtype == XCB_MOTION_NOTIFY) {
			time = copyu<xcb_motion_notify_event_t>(ev->event).time;
		} else if(rtype == XCB_KEY_PRESS) {
			time = copyu<xcb_key_press_event_t>(ev->event).time;
		} else if(rtype == XCB_KEY_RELEASE) {
			time = copyu<xcb_key_release_event_t>(ev->event).time;
		}
	}

	// try to take ownership of the xdnd selection
	auto xdndSelection = atoms().xdndSelection;
	xcb_set_selection_owner(&xConnection(), xDummyWindow(), xdndSelection,
		appContext().time());

	// check for success
	auto owner = selectionOwner(xdndSelection);
	if(owner != xDummyWindow()) {
		dlg_warn("startDragDrop: failed to set selction owner");
		return false;
	}

	// grab the pointer
	auto wc = dynamic_cast<X11WindowContext*>(appContext().mouseContext()->over());
	auto window = appContext().xDefaultScreen().root;

	xcb_cursor_t cursor = XCB_NONE;
	if(src->supportedActions() & DndAction::copy) {
		cursor = cursors_.dndCopy;
	} else if(src->supportedActions() & DndAction::move) {
		cursor = cursors_.dndCopy;
	}

	auto grabCookie = xcb_grab_pointer(&xConnection(), false, window,
		dndGrabEventMask, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE,
		cursor, time);

	xcb_generic_error_t* errorPtr {};
	auto grabReply = xcb_grab_pointer_reply(&xConnection(), grabCookie, &errorPtr);
	if(errorPtr) {
		auto msg = x11::errorMessage(appContext().xDisplay(), errorPtr->error_code);
		dlg_warn("xcb_grab_pointer error: {}", msg);
		free(errorPtr);

		// give up the selection
		xcb_set_selection_owner(&xConnection(), xDummyWindow(), xdndSelection,
			appContext().time());
		return false;
	}

	auto result = static_cast<unsigned int>(grabReply->status);
	free(grabReply);

	if(result != XCB_GRAB_STATUS_SUCCESS) {
		dlg_warn("grabbing cursor returned {}", result);

		// give up the selection
		xcb_set_selection_owner(&xConnection(), xDummyWindow(), xdndSelection,
			appContext().time());
		return false;
	}

	// render and show the dnd window
	auto img = src->image();
	if(img.format != ImageFormat::none) {
		dndSrc_.dndWindow->size(img.size);
		dndSrc_.dndWindow->show();

		{
			auto surf = dndSrc_.dndWindow->surface().buffer;
			dlg_assert(surf);
			auto guard = surf->buffer();
			auto buf = guard.get();
			dlg_assertm(buf.size == img.size, "{} vs {}", buf.size, img.size);
			convertFormatStride(img, buf.format, *buf.data, buf.stride);
		}
	}

	// if everything went fine, store the DataSource implementation to be able to
	// answer requests we might get
	// storing a valid DataSource in dndSource also effects that we will handle pointer event
	// of the grabbed pointer
	dndSrc_.source = {appContext(), std::move(src), time};
	dndSrc_.sourceWindow = wc->xWindow();

	// if the data source supportes more than 3 targets, we can already set the xdndtypelist
	// property of the source window
	const auto& targets = dndSrc_.source.targets();
	if(targets.size() > 3) {
		xcb_change_property(&xConnection(), XCB_PROP_MODE_REPLACE, dndSrc_.sourceWindow,
			atoms().xdndTypeList, XCB_ATOM_ATOM, 32, targets.size(), targets.data());
	}

	return true;
}

xcb_window_t X11DataManager::selectionOwner(xcb_atom_t selection) {
	xcb_generic_error_t* error {};
	auto ownerCookie = xcb_get_selection_owner(&xConnection(), selection);
	auto reply = xcb_get_selection_owner_reply(&xConnection(), ownerCookie, &error);

	xcb_window_t owner {};
	if(error) {
		auto msg = x11::errorMessage(appContext().xDisplay(), error->error_code);
		dlg_warn("xcb_get_selection_owner: {}", msg);
		free(error);
	}
	else if(reply) {
		owner = reply->owner;
		free(reply);
	}

	return owner;
}

void X11DataManager::unregisterDataOffer(const X11DataOffer& offer) {
	auto it = std::find(oldDndOffers_.begin(), oldDndOffers_.end(), &offer);
	if(it == oldDndOffers_.end()) {
		dlg_warn("invalid offer {} given", (void*) &offer);
		return;
	}

	oldDndOffers_.erase(it);
}

xcb_connection_t& X11DataManager::xConnection() const {
	return appContext().xConnection();
}

xcb_window_t X11DataManager::xDummyWindow() const {
	return appContext().xDummyWindow();
}

const x11::Atoms& X11DataManager::atoms() const {
	return appContext().atoms();
}

void X11DataManager::xdndSendEnter() {
	const auto& targets = dndSrc_.source.targets();

	// see the xdnd spec for more information
	// we only send the supported targets if it aren't more than 3
	// otherwise we stored it in the xdndTypeList property in the startDragDrop function
	xcb_client_message_event_t clientev {};
	clientev.window = dndSrc_.targetWindow;
	clientev.format = 32;
	clientev.response_type = XCB_CLIENT_MESSAGE;
	clientev.type = atoms().xdndEnter;
	clientev.data.data32[0] = dndSrc_.sourceWindow;
	clientev.data.data32[1] = (targets.size() > 3) | (dndSrc_.version << 24);

	if(targets.size() <= 3) {
		if(targets.size() > 0) {
			clientev.data.data32[2] = targets[0];
			if(targets.size() > 1) {
				clientev.data.data32[3] = targets[1];
				if(targets.size() > 2) {
					clientev.data.data32[4] = targets[2];
				}
			}
		}
	}

	auto eventPtr = reinterpret_cast<const char*>(&clientev);
	xcb_send_event(&xConnection(), 0, dndSrc_.targetWindow, 0, eventPtr);
}

void X11DataManager::xdndSendLeave() {
	xcb_client_message_event_t clientev {};
	clientev.window = dndSrc_.targetWindow;
	clientev.format = 32;
	clientev.response_type = XCB_CLIENT_MESSAGE;
	clientev.type = atoms().xdndLeave;
	clientev.data.data32[0] = dndSrc_.sourceWindow;

	auto eventPtr = reinterpret_cast<const char*>(&clientev);
	xcb_send_event(&xConnection(), 0, dndSrc_.targetWindow, 0, eventPtr);
}

void X11DataManager::xdndSendPosition(nytl::Vec2i rootPos, xcb_timestamp_t time) {
	xcb_client_message_event_t clientev {};
	clientev.window = dndSrc_.targetWindow;
	clientev.format = 32;
	clientev.response_type = XCB_CLIENT_MESSAGE;
	clientev.type = atoms().xdndPosition;
	clientev.data.data32[0] = dndSrc_.sourceWindow;
	clientev.data.data32[1] = 0u; // reserved
	clientev.data.data32[2] = (rootPos[0] << 16) | rootPos[1];
	if(dndSrc_.version >= 1) {
		clientev.data.data32[3] = time;
	}
	if(dndSrc_.version >= 2) {
		xcb_atom_t actionAtom = 0;
		auto actions = dndSrc_.source.dataSource().supportedActions();
		if(actions & DndAction::copy) {
			actionAtom = atoms().xdndActionCopy;
		} else if(actions & DndAction::move) {
			actionAtom = atoms().xdndActionMove;
		}
		clientev.data.data32[4] = actionAtom;
	}

	auto eventPtr = reinterpret_cast<const char*>(&clientev);
	xcb_send_event(&xConnection(), 0, dndSrc_.targetWindow, 0, eventPtr);
}

void X11DataManager::destroyed(const X11WindowContext& wc) {
	// check if there is a dnd offer over the destroyed context.
	// in that case simply forget everything about it
	if(&wc == dndOffer_.windowContext) {
		dndOffer_ = {};
	}

	// if there was a dnd request over the destroyed window context
	// destroy it. If it's currently over another window, notify that.
	if(auto xwin = wc.xWindow(); xwin && xwin == dndSrc_.sourceWindow) {
		if(dndSrc_.targetWindow) {
			xdndSendLeave();
		}

		xcb_ungrab_pointer(&xConnection(), XCB_CURRENT_TIME);
		dndSrc_.dndWindow->hide();
		dndSrc_.version = {};
		dndSrc_.targetWindow = {};
		dndSrc_.sourceWindow = {};
		dndSrc_.pendingStatus_ = {};
		dndSrc_.sendPos = {};
	}
}

} // namespace ny
