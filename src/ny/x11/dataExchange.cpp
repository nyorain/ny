// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/dataExchange.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/input.hpp>
#include <ny/x11/bufferSurface.hpp>
#include <ny/asyncRequest.hpp>
#include <ny/bufferSurface.hpp>
#include <dlg/dlg.hpp>
#include <nytl/vecOps.hpp>

#include <algorithm>

// the data manager was modeled after the clipboard specification of iccccm
// https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#use_of_selection_atoms

// TODO: something about notify event timeout (or leave it to application?)
// probably best done using a timerfd and fd callback support in x11appcontext
// TODO: timestamps currently not implemented like in icccm convention (NOT XCB_CURRENT_TIME)
// TODO: should we surrender owned selections on X11DataManager destruction?
// TODO: unregister/unsert X11DataManager::currentDndWC_ when it is destroyed
// TODO: correctly set cursor, handle DataSource image (create dnd window)
// TODO: xdnd enter handle target atoms set to None (i.e. ignore them)
// TODO: DataManager: better event proccessing (connect dnd session to input?) currently more a hack
// TODO: handle actions (xdndposition)
// TODO: handle xdndstatus
// TODO: xdndproxy
// TODO: split up the multiple hundred lines of dnd/selection event handling

namespace ny {
namespace {

/// Checks if the given window is dnd aware. Returns the xdnd protocol version that should
/// be used to communicate with the window or 0 if none is supported.
unsigned int xdndAware(X11AppContext& ac, xcb_window_t window)
{
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

// - Classes -
// small DefaultAsyncRequest derivate that will unregister itself from pending requests
// on destruction
template<typename T>
class X11DataOffer::AsyncRequestImpl : public DefaultAsyncRequest<T> {
public:
	using DefaultAsyncRequest<T>::DefaultAsyncRequest;
	nytl::UniqueConnection connection;
};

// X11AsyncRequest derivate that will be used for data requests that first have to wait
// for the supported formats request to complete
class X11DataOffer::DataFormatRequestImpl : public AsyncRequestImpl<std::any> {
public:
	using AsyncRequestImpl<std::any>::AsyncRequestImpl;
	DataOffer::FormatsRequest formatsRequest;
};


// - Implementation
// DataOffer
X11DataOffer::X11DataOffer(X11AppContext& ac, unsigned int selection, xcb_window_t owner)
	: appContext_(&ac), selection_(selection), owner_(owner)
{
	// ask the selection owner to enumerate targets, i.e. to send us all supported format
	xcb_convert_selection(&appContext().xConnection(), appContext().xDummyWindow(), selection_,
		appContext().atoms().targets, appContext().atoms().clipboard, XCB_CURRENT_TIME);
}

X11DataOffer::X11DataOffer(X11AppContext& ac, unsigned int selection, xcb_window_t owner,
	nytl::Span<const xcb_atom_t> supportedTargets)
		: appContext_(&ac), selection_(selection), owner_(owner)
{
	// just parse/add all supported target formats.
	// addFormats sets formatsRetrieved_ to true, so in formats() we can
	// directly return a completed FormatsRequest
	addFormats(supportedTargets);
}

X11DataOffer::~X11DataOffer()
{
	// signal all pending format requests that they have failed
	// this will unregister the callbacks and destroy the connections
	pendingFormatRequests_({});

	// singal all pending data requests that they have failed
	// this will unregister the callbacks and destroy the connections
	for(auto& pdr : pendingDataRequests_) pdr.second({});

	// if the Application had ownership over this DataOffer we have to unregister from
	// the DataManager so no further selection notifty events are dispatched to us
	if(unregister_) appContext().dataManager().unregisterDataOffer(*this);
}

X11DataOffer::FormatsRequest X11DataOffer::formats()
{
	using RequestImpl = AsyncRequestImpl<std::vector<DataFormat>>;

	// check if formats have already been retrieved
	if(formatsRetrieved_) {
		std::vector<DataFormat> fmts {};
		fmts.reserve(formats_.size());
		for(auto& fmt : formats_) fmts.push_back(fmt.first);

		return std::make_unique<RequestImpl>(std::move(fmts));
	}

	auto ret = std::make_unique<RequestImpl>(appContext());

	// the returned request will unregister the callback automatically on destruction,
	// therefore we can capute it and use it inside the callback.
	// the request will additionally be completed (with failure) when this DataOffer is
	// destructed
	// we also make sure that the connection is destroyed when this callback is called, therefore
	// it will not be destroyed from the connection guard on destruction (where the DataOffer and
	// therefore the associated callback might already be destroyed).
	ret->connection = pendingFormatRequests_.add([req = ret.get()](std::vector<DataFormat> fmts) {
		req->complete(std::move(fmts));
		req->connection = {}; //disconnects and makes sure it will not disconnect on destruction
	});

	return ret;
}

X11DataOffer::DataRequest X11DataOffer::data(const DataFormat& fmt)
{
	// if we did not even retrieve the supported formats we actually have to create
	// an extra request for the format
	if(!formatsRetrieved_) {
		auto ret = std::make_unique<DataFormatRequestImpl>(appContext());
		ret->formatsRequest = formats();

		// the returned request will unregister the callback on destruction and when
		// this DataOffer is destroyed, therefore both can be accessed (both are not movable)
		// this callback is triggered when the formats are retrieved and it will (try to) request
		// data in the request format.
		ret->formatsRequest->callback([fmt, this, req = ret.get()](const auto&){
			auto connection = this->registerDataRequest(fmt, *req);
			if(connection.connected()) req->connection = std::move(connection);

			// if connection is empty, this->registerDataRequest did already complete
			// the request with an empty any data object
		});

		return ret;
	}

	// Just return a default data request that is waiting for the data to arrive
	auto ret = std::make_unique<AsyncRequestImpl<std::any>>(appContext());
	ret->connection = registerDataRequest(fmt, *ret);
	if(!ret->connection.connected()) return {};
	return ret;
}

nytl::UniqueConnection X11DataOffer::registerDataRequest(const DataFormat& format,
	AsyncRequestImpl<std::any>& request)
{
	// check if the requested format is supported at all and query the associated
	// target format atom
	xcb_atom_t target = 0u;
	for(auto& mapping : formats_) {
		if(mapping.first == format) {
			target = mapping.second;
			break;
		}
	}

	// if the format is not supported complete the request with an empty data object and return
	// an empty connection
	if(!target) {
		request.complete({});
		return {};
	}

	// request the data in the target format from the selection owner that offers the data
	// we request the owner to store it into the clipboard atom property of the dummy window
	auto cookie = xcb_convert_selection_checked(&appContext().xConnection(),
		appContext().xDummyWindow(), selection_, target,
		appContext().atoms().clipboard, XCB_CURRENT_TIME);

	appContext().errorCategory().checkWarn(cookie, "ny::X11DataOffer::registerDataRequest:0");

	// add a callback to the pending data callbacks that will be triggered as soon
	// as we receive the data in the requested format this callback will unregister itself.
	// we also make sure that the connection is destroyed when this callback is called, therefore
	// it will not be destroyed from the connection guard on destruction (where the DataOffer and
	// therefore the associated callback might already be destroyed).
	return pendingDataRequests_[target].add([req = &request](const std::any& any) {
		req->complete(any);
		req->connection = {}; //disconnects
	});
}

void X11DataOffer::notify(const xcb_selection_notify_event_t& notify)
{
	// if the property is 0 the request failed
	if(notify.property == 0) {
		dlg_info("request failed (property == 0)");
		return;
	}

	xcb_generic_error_t error {};
	auto prop = x11::readProperty(appContext().xConnection(), notify.property,
		appContext().xDummyWindow(), &error);

	if(error.error_code || prop.data.empty()) {
		auto msg = std::string("No property data was returned");
		if(error.error_code) msg = x11::errorMessage(appContext().xDisplay(), error.error_code);
		dlg_info("failed to read the target property: {}", msg);
		return;
	}

	// check the target of the notify
	// if the target it atoms.target, it notifies us that the selection owner set the
	// property of the dummy window to the list of supported (convertable targets)
	// otherwise it sets it to the data of some specific requested target
	if(notify.target == appContext().atoms().targets) {
		if(formatsRetrieved_) return;

		//the property must be set to a list of atoms which have 32 bit length
		//so if the format is not 32, the property/selectionEvent is invalid
		if(prop.format != 32 || prop.data.empty()) {
			dlg_info("received targets notify with invalid property data");
			return;
		}

		// add the retrieved atoms the the list of supported targets/formats
		auto* targetAtoms = reinterpret_cast<const xcb_atom_t*>(prop.data.data());
		auto size = static_cast<unsigned int>(prop.data.size() / 4);
		addFormats({targetAtoms, size});

		// extract the supported data formats
		std::vector<DataFormat> supportedFormats;
		supportedFormats.reserve(formats_.size());
		for(auto& fmt : formats_) supportedFormats.push_back(fmt.first);

		// complete the pending format requests
		pendingFormatRequests_(std::move(supportedFormats));
		pendingFormatRequests_.clear();
	} else {
		auto target = notify.target;
		auto it = pendingDataRequests_.find(target);
		if(it == pendingDataRequests_.end()) {
			dlg_info("received notify with unkown/not requested target");
			return;
		}

		// find the associated dataType format
		const auto* format = &DataFormat::none;
		for(const auto& fmt : formats_) {
			if(fmt.second == target) {
				format = &fmt.first;
				break;
			}
		}

		if(*format == DataFormat::none) {
			dlg_warn("format not supported - internal inconsistency");
			it->second({}); // was invalid all the time? how could this happen?
			return;
		}

		// construct an any data object for the raw buffer
		// and complete the pending data requests for this data format
		auto any = wrap(prop.data, *format);
		it->second(std::move(any));
		pendingDataRequests_.erase(it);
	}
}

void X11DataOffer::addFormats(nytl::Span<const xcb_atom_t> targets)
{
	// TODO: filter out special formats such as MULTIPLE or TIMESTAMP or stuff
	// they should not be advertised to the application

	std::vector<std::pair<xcb_get_atom_name_cookie_t, xcb_atom_t>> atomNameCookies;
	atomNameCookies.reserve(targets.size());

	auto& atoms = appContext().atoms();
	auto& xConn = appContext().xConnection();

	// check for known target atoms
	// if the target atom is not known, push a cookie to request its name
	for(auto& target : targets) {
		if(target == atoms.utf8string) formats_.emplace(DataFormat::text, target);
		else if(target == XCB_ATOM_STRING) formats_.emplace(DataFormat::text, target);
		else if(target == atoms.text) formats_.emplace(DataFormat::text, target);
		else if(target == atoms.mime.textPlain) formats_.emplace(DataFormat::text, target);
		else if(target == atoms.mime.textPlainUtf8) formats_.emplace(DataFormat::text, target);
		else if(target == atoms.fileName) formats_.emplace(DataFormat::uriList, target);
		else if(target == atoms.mime.imageData) formats_.emplace(DataFormat::image, target);
		else if(target == atoms.mime.raw) formats_.emplace(DataFormat::raw, target);
		else atomNameCookies.push_back({xcb_get_atom_name(&xConn, target), target});
	}

	// try to get all requested names and insert them into formats_ if successful
	for(auto& cookie : atomNameCookies) {
		xcb_generic_error_t* error {};
		auto nameReply = xcb_get_atom_name_reply(&xConn, cookie.first, &error);

		if(error) {
			auto msg = x11::errorMessage(appContext().xDisplay(), error->error_code);
			dlg_warn("get_atom_name_reply failed: {}", msg);
			free(error);
			continue;
		} else if(!nameReply) {
			dlg_warn("get_atom_name_reply failed: without error");
			continue;
		}

		auto name = xcb_get_atom_name_name(nameReply);
		auto string = std::string(name, name + xcb_get_atom_name_name_length(nameReply));
		free(nameReply);
		formats_[{std::move(string)}] = cookie.second;
	}

	// remember that we have the formats retrieved, i.e. formats_ it complete now
	// so we don't request them again
	formatsRetrieved_ = true;
}

// X11DataSource
X11DataSource::X11DataSource(X11AppContext& ac, std::unique_ptr<DataSource> src)
	: appContext_(&ac), dataSource_(std::move(src))
{
	// convert the supported formats of the source to target atoms
	auto& atoms = appContext().atoms();
	auto& xConn = appContext().xConnection();
	auto formats = dataSource_->formats();

	// TODO: care about duplicates?

	std::vector<std::pair<const DataFormat*, xcb_intern_atom_cookie_t>> atomCookies;
	atomCookies.reserve(formats.size());

	//check for known special formats
	for(auto& fmt : formats) {
		if(fmt == DataFormat::text) {
			formatsMap_.push_back({atoms.utf8string, fmt});
			formatsMap_.push_back({atoms.mime.textPlainUtf8, fmt});
			formatsMap_.push_back({XCB_ATOM_STRING, fmt});
			formatsMap_.push_back({atoms.text, fmt});
			formatsMap_.push_back({atoms.mime.textPlain, fmt});
		} else if(fmt == DataFormat::uriList) {
			formatsMap_.push_back({atoms.utf8string, fmt});
			formatsMap_.push_back({atoms.mime.textPlainUtf8, fmt});
			formatsMap_.push_back({XCB_ATOM_STRING, fmt});
			formatsMap_.push_back({atoms.text, fmt});
			formatsMap_.push_back({atoms.mime.textPlain, fmt});
			formatsMap_.push_back({atoms.mime.textUriList, fmt});

			// TODO:
			// check if the uri list is only one file, then we can support the filename target
		} else {
			auto atomCookie = xcb_intern_atom(&xConn, 0, fmt.name.size(), fmt.name.c_str());
			atomCookies.push_back({&fmt, atomCookie});

			for(auto& an : fmt.additionalNames) {
				auto atomCookie = xcb_intern_atom(&xConn, 0, an.size(), an.c_str());
				atomCookies.push_back({&fmt, atomCookie});
			}
		}
	}

	// receive all atoms for the supported formats and insert them into the
	// vector of supported formats
	for(auto& cookie : atomCookies) {
		xcb_generic_error_t* error {};
		auto reply = xcb_intern_atom_reply(&xConn, cookie.second, &error);
		if(reply) {
			formatsMap_.push_back({reply->atom, *cookie.first});
			free(reply);
			continue;
		} else if(error) {
			auto msg = x11::errorMessage(appContext().xDisplay(), error->error_code);
			dlg_warn("Failed to load atom for {}", cookie.first->name);
			free(error);
		}
	}

	// extract a vector of supported targets since this is used pretty often
	// also remove duplicates from the targets vector
	targets_.reserve(formatsMap_.size());
	for(const auto& entry : formatsMap_)
		targets_.push_back(entry.first);

	std::sort(targets_.begin(), targets_.end());
	targets_.erase(std::unique(targets_.begin(), targets_.end()), targets_.end());
}

void X11DataSource::answerRequest(const xcb_selection_request_event_t& request)
{
	// TODO: correctly implement all (reasonable parts) of icccm
	auto property = request.property;
	if(!property) property = request.target;

	if(request.target == appContext().atoms().targets) {
		// set the requested property of the requested window to the target atoms
		xcb_change_property(&appContext().xConnection(), XCB_PROP_MODE_REPLACE, request.requestor,
			property, XCB_ATOM_ATOM, 32, targets_.size(), targets_.data());
	} else {
		// let the source convert the data to the request type and send it (store as property)
		const DataFormat* format {};
		for(const auto& fmt : formatsMap_) {
			if(fmt.first == request.target) {
				format = &fmt.second;
				break;
			}
		}

		if(!format) {
			dlg_info("unsupported target request");
			property = 0u;
		} else {
			// request the data in the associated DataFormat from the source
			auto any = dataSource_->data(*format);
			if(!any.has_value()) {
				dlg_warn("data source could not provide data");
				// TODO: set property and break here or sth.
			}

			// convert the value of the any to a raw buffer in some way
			auto atomFormat = 8u;
			auto type = XCB_ATOM_STRING;
			auto ownedBuffer = unwrap(any, *format);

			// set the property of the requested window to the raw data
			xcb_change_property(&appContext().xConnection(), XCB_PROP_MODE_REPLACE,
				request.requestor, property, type, atomFormat,
				ownedBuffer.size(), ownedBuffer.data());
		}
	}

	// notify the requestor
	xcb_selection_notify_event_t notifyEvent {};
	notifyEvent.response_type = XCB_SELECTION_NOTIFY;
	notifyEvent.selection = request.selection;
	notifyEvent.property = property;
	notifyEvent.target = request.target;
	notifyEvent.requestor = request.requestor;

	auto eventPtr = reinterpret_cast<const char*>(&notifyEvent);
	xcb_send_event(&appContext().xConnection(), 0, request.requestor, 0, eventPtr);
	xcb_flush(&appContext().xConnection());
}

// X11DataManager
X11DataManager::X11DataManager(X11AppContext& ac) : 
	appContext_(&ac)
{
	X11WindowSettings settings;
	settings.transparent = true;
	settings.droppable = false;
	settings.show = true;
	settings.surface = SurfaceType::buffer;
	settings.size = {32, 32};

	settings.overrideRedirect = true;
	settings.windowType = ac.ewmhConnection()._NET_WM_WINDOW_TYPE_DND;
	dndSrc_.dndWindow = std::make_unique<X11BufferWindowContext>(ac, settings);
}

bool X11DataManager::processEvent(const xcb_generic_event_t& ev)
{
	X11EventData eventData {ev};

	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
		case XCB_SELECTION_NOTIFY: {
			// a selection owner notifies us that it set the request property
			// we need to query the DataOffer this is associated with and let it handle
			// the notification
			auto& notify = reinterpret_cast<const xcb_selection_notify_event_t&>(ev);
			auto& atoms = appContext().atoms();

			if(notify.requestor != appContext().xDummyWindow()) {
				dlg_info("received selection notify for invalid window");
				return true;
			}

			X11DataOffer* dataOffer {};
			if(notify.selection == atoms.clipboard) {
				dataOffer = clipboardOffer_.get();
			} else if(notify.selection == XCB_ATOM_PRIMARY) {
				dataOffer = primaryOffer_.get();
			} else if(notify.selection == atoms.xdndSelection)
			{
				dataOffer = dndOffer_.offer.get();
				if(!dataOffer && !dndOffers_.empty()) dataOffer = dndOffers_.front();

				// TODO:
				// make it possible to identify the dnd offer this notification belongs to.
				// remove the pile of shit above

				// for(auto& dndOffer : dndOffers_)
				// {
				// }
			}

			if(dataOffer) dataOffer->notify(notify);
			else dlg_info("received selection notify for invalid selection");

			return true;
		}

		case XCB_SELECTION_REQUEST: {
			// some other x clients requests us to convert a selection or send him
			// the supported targets
			// find the associated data source and let it answer the request
			auto& req = reinterpret_cast<const xcb_selection_request_event_t&>(ev);

			X11DataSource* source = nullptr;
			if(req.selection == atoms().clipboard) source = &clipboardSource_;
			else if(req.selection == XCB_ATOM_PRIMARY) source = &primarySource_;
			else if(req.selection == atoms().xdndSelection) source = &dndSrc_.source;

			if(source) {
				source->answerRequest(req);
			} else {
				//if the request cannot be handled, simply send an event to the client
				//that notifies him the request failed.
				dlg_info("received unknown selection request");

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
		}

		case XCB_SELECTION_CLEAR: {
			// we are notified that we lost ownership over a selection
			// query the associated data source and unset it
			auto& clear = reinterpret_cast<const xcb_selection_clear_event_t&>(ev);
			if(clear.owner != xDummyWindow()) {
				dlg_info("received selection clear event for invalid window");
				return true;
			}

			if(clear.selection == atoms().clipboard) clipboardSource_ = {};
			else if(clear.selection == XCB_ATOM_PRIMARY) primarySource_ = {};

			return true;
		}

		// xdnd events are sent as client messages
		case XCB_CLIENT_MESSAGE: {
			auto& clientm = reinterpret_cast<const xcb_client_message_event_t&>(ev);
			if(processClientMessage(clientm, eventData)) return true;
		}

		default:
			break;
	}

	// if we are currently grabbing the pointer because we initiated a dnd session
	// also handle the pointer events and forward them to dnd handlers.
	if(dndSrc_.sourceWindow && processDndEvent(ev)) return true;
	return false; // we did never handle the event in any way
}

bool X11DataManager::processClientMessage(const xcb_client_message_event_t& clientm,
	const EventData& eventData)
{
	if(clientm.type == atoms().xdndEnter) {
		auto* data = clientm.data.data32;

		bool moreThan3 = data[1] & 1; // whether the supported targets are attached
		xcb_window_t source = data[0]; // the source of the dnd operation
		auto protocolVersion = data[1] >> 24; // the supported protocol version

		std::vector<xcb_atom_t> targets;
		targets.reserve(10);

		if(!moreThan3) {
			// extract the supported targets from the event
			if(data[2]) targets.push_back(data[2]);
			if(data[3]) targets.push_back(data[3]);
			if(data[4]) targets.push_back(data[4]);
		} else {
			// read the xdndtypelist atom property of the source window
			// for the supported targets
			xcb_generic_error_t error {};
			auto typeList = x11::readProperty(xConnection(), atoms().xdndTypeList,
				source, &error);

			if(error.error_code) {
				auto msg = x11::errorMessage(appContext().xDisplay(), error.error_code);
				dlg_info("reading xdndTypeList property failed: {}", msg);
				return true;
			}

			auto begin = reinterpret_cast<xcb_atom_t*>(typeList.data.data());
			auto end = begin + (typeList.data.size() / 4);
			targets.insert(targets.end(), begin, end);
		}

		// getting the associated WindowContext
		auto windowContext = appContext().windowContext(clientm.window);
		if(!windowContext) {
			dlg_info("xdndEnter for invalid x window");
			return true;
		}

		// create a new data offer and store it as current one
		// also store the protocol version used for the current session
		auto offer = std::make_unique<X11DataOffer>(appContext(),
			atoms().xdndSelection, source, targets);

		dndOffer_.version = protocolVersion;
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

		// the position is given relative to the root window so we have to
		// translate it first
		auto rootwin = appContext().xDefaultScreen().root;
		auto cookie = xcb_translate_coordinates(&xConnection(), rootwin, clientm.window,
			clientm.data.data32[2] >> 16, clientm.data.data32[2] & 0xffff);

		xcb_generic_error_t* error {};
		auto reply = xcb_translate_coordinates_reply(&xConnection(), cookie, &error);
		if(error) {
			auto msg = x11::errorMessage(appContext().xDisplay(), error->error_code);
			dlg_info("xdndPosition: xcb_translate_coordinates: {}", msg);
			free(error);
			return true;
		}

		nytl::Vec2i pos{reply->dst_x, reply->dst_y};
		free(reply);

		DndMoveEvent dme;
		dme.eventData = &eventData;
		dme.position = pos;
		dme.offer = dndOffer_.offer.get();
		auto format = dndOffer_.windowContext->listener().dndMove(dme);

		bool accepted = (format != DataFormat::none);

		// answer with the current state, i.e. whether the application can/will accept
		// the offer or not
		xcb_client_message_event_t revent {};
		revent.response_type = XCB_CLIENT_MESSAGE;
		revent.window = clientm.data.data32[0];
		revent.type = atoms().xdndStatus;
		revent.format = 32u;
		revent.data.data32[0] = clientm.window;
		revent.data.data32[1] = accepted;
		revent.data.data32[2] = 0;
		revent.data.data32[3] = 0;
		revent.data.data32[4] = atoms().xdndActionCopy;

		auto reventPtr = reinterpret_cast<char*>(&revent);
		xcb_send_event(&xConnection(), 0, revent.window, 0, reventPtr);
		xcb_flush(&xConnection());
	} else if(clientm.type == atoms().xdndLeave) {
		if(!dndOffer_.windowContext || !dndOffer_.offer) {
			dlg_info("xdndLeave event without current xdnd session");
		} else {
			DndLeaveEvent dle;
			dle.eventData = &eventData;
			dle.offer = dndOffer_.offer.get();
			dndOffer_.windowContext->listener().dndLeave(dle);
		}

		// reset the currently active data offer and all its data
		dndOffer_ = {};
	} else if(clientm.type == atoms().xdndDrop) {
		// generate a data offer event
		// push the current data offer into the vector vector with old ones
		if(!dndOffer_.windowContext || !dndOffer_.offer) {
			dlg_info("xdndDrop event without current xdnd session");
			return true;
		}

		// TODO: send event if application signaled that it cannot handle the dataOffer?
		// do not generate dndDrop listener event in this case?

		nytl::Vec2i pos {};
		pos[0] = clientm.data.data32[2] >> 16;
		pos[1] = clientm.data.data32[2] & 0xffff;

		dndOffers_.push_back(dndOffer_.offer.get());
		dndOffer_.offer->unregister();

		DndDropEvent dde;
		dde.position = pos;
		dde.offer = std::move(dndOffer_.offer);
		dde.eventData = &eventData;
		dndOffer_.windowContext->listener().dndDrop(dde);

		// reset offer
		dndOffer_ = {};

	} else if(clientm.type == atoms().xdndStatus) {
		// we (as drag source) received a status message
		// mainly interesting for debugging

		// auto accepted = (clientm.data.data32[1] & 1u);
		// debug("accepted: ", accepted);
		//
		// auto action = clientm.data.data32[4];
		// auto name = "<unknown>";
		// if(action == atoms().xdndActionAsk) name = "ask";
		// if(action == atoms().xdndActionCopy) name = "copy";
		// if(action == atoms().xdndActionMove) name = "move";
		// if(action == atoms().xdndActionLink) name = "link";
		// if(action == 0) name = "none";
		// debug("action: ", name);
	} else {
		return false; // if we land here we could not handle the client message
	}

	return true; // we handled the client message
}

bool X11DataManager::processDndEvent(const xcb_generic_event_t& ev)
{
	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
		case XCB_MOTION_NOTIFY: {
			const auto& motionEv = reinterpret_cast<const xcb_motion_notify_event_t&>(ev);

			// query the xdnd window we are currently over
			// this here is a real performance killer, so optimizations like caching
			// the size are welcome
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

			// move the dnd window, TODO: offset (and XQueryTree, correct querying)
			uint32_t values2[] = {(uint32_t) motionEv.root_x + 5, (uint32_t) motionEv.root_y + 5};
			xcb_configure_window(&xConnection(), dndSrc_.dndWindow->xWindow(), 
				XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
				values2);

			// send leave event to old one and enter to new one if valid if it changed
			// otherwise just send a default position event
			if(toplevel != dndSrc_.targetWindow) {
				if(dndSrc_.targetWindow) {
					xdndSendLeave();
				}

				dndSrc_.targetWindow = toplevel;
				dndSrc_.version = version;
				if(dndSrc_.targetWindow) {
					xdndSendEnter();
				}
			} else if(dndSrc_.targetWindow) {
				xdndSendPosition({motionEv.root_x, motionEv.root_y});
			}

			return true;
		}

		case XCB_BUTTON_RELEASE: {
			if(dndSrc_.targetWindow) {

				// if we are over an xdnd window, send a drop event
				xcb_client_message_event_t clientev {};
				clientev.window = dndSrc_.targetWindow;
				clientev.format = 32;
				clientev.response_type = XCB_CLIENT_MESSAGE;
				clientev.type = atoms().xdndDrop;
				clientev.data.data32[0] = dndSrc_.sourceWindow;

				auto eventPtr = reinterpret_cast<const char*>(&clientev);
				xcb_send_event(&xConnection(), 0, dndSrc_.targetWindow, 0, eventPtr);
			}

			// end the dnd session, i.e. ungrab the pointer
			// send a xdnddrop event if we are over an accepting window
			xcb_ungrab_pointer(&xConnection(), XCB_CURRENT_TIME);

			// reset all session values
			// not yet reset the dnd source sine it might be needed further
			dndSrc_.version = {};
			dndSrc_.sourceWindow = {};
			dndSrc_.targetWindow = {};
			dndSrc_.dndWindow->hide();

			return true;
		}

		default: break;
	}

	return false;
}

bool X11DataManager::clipboard(std::unique_ptr<DataSource>&& dataSource)
{
	// try to get ownership of selection
	xcb_set_selection_owner(&xConnection(), xDummyWindow(), atoms().clipboard, XCB_CURRENT_TIME);

	// check for success
	// icccm specifies that it may fail and should only be considered succesful if
	// the owner is really changed
	auto owner = selectionOwner(atoms().clipboard);
	if(owner != xDummyWindow()) return false;

	clipboardSource_ = {appContext(), std::move(dataSource)};
	return true;
}

DataOffer* X11DataManager::clipboard()
{
	auto owner = selectionOwner(atoms().clipboard);

	// if there is no owner, the clipboard has currently no value
	// we also (impliclty) reset the clipboard data offer then because it should no longer
	// be used.
	if(!owner) {
		clipboardOffer_ = {};
		return nullptr;
	}

	//refresh the clipboard offer if needed to do so.
	if(!clipboardOffer_ || owner != clipboardOffer_->owner())
		clipboardOffer_ = std::make_unique<X11DataOffer>(appContext(), atoms().clipboard, owner);

	return clipboardOffer_.get();
}

bool X11DataManager::startDragDrop(std::unique_ptr<DataSource> src)
{
	// TODO: better check, give up xdndselection ownership on fail
	// check if currently over window
	if(!appContext().mouseContext()->over()) {
		dlg_warn("startDragDrop: not having mouse focus");
		return false;
	}

	// try to take ownership of the xdnd selection
	auto xdndSelection = atoms().xdndSelection;
	xcb_set_selection_owner(&xConnection(), xDummyWindow(), xdndSelection, XCB_CURRENT_TIME);

	// check for success
	auto owner = selectionOwner(xdndSelection);
	if(owner != xDummyWindow()) {
		dlg_warn("startDragDrop: failed to set selction owner");
		return false;
	}

	// grab the pointer
	auto eventMask = XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW |
		XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION;

	auto wc = dynamic_cast<X11WindowContext*>(appContext().mouseContext()->over());
	auto window = appContext().xDefaultScreen().root;

	auto grabCookie = xcb_grab_pointer(&xConnection(), false, window, eventMask,
		XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);

	xcb_generic_error_t* errorPtr {};
	auto grabReply = xcb_grab_pointer_reply(&xConnection(), grabCookie, &errorPtr);
	if(errorPtr) {
		auto msg = x11::errorMessage(appContext().xDisplay(), errorPtr->error_code);
		dlg_warn("xcb_grab_pointer error: {}", msg);
		free(errorPtr);
		return false;
	}

	auto result = static_cast<unsigned int>(grabReply->status);
	free(grabReply);

	if(result != XCB_GRAB_STATUS_SUCCESS) {
		dlg_warn("grabbing cursor returned {}", result);
		return false;
	}

	// render and show the dnd window
	auto img = src->image();
	if(img.format != ImageFormat::none) {
		// TODO: temporary hack
		dndSrc_.dndWindow->size(img.size);
		dndSrc_.dndWindow->updateSize(img.size);
		dndSrc_.dndWindow->show();

		{
			auto surf = dndSrc_.dndWindow->surface().buffer;
			dlg_assert(surf);
			auto guard = surf->buffer();
			auto buf = guard.get();
			dlg_assertm(buf.size == img.size, "{} vs {}", buf.size, img.size);
			convertFormat(img, buf.format, *buf.data); // TODO: stride?
		}
	}

	// if everything went fine, store the DataSource implementation to be able to
	// answer requests we might get
	// storing a valid DataSource in dndSource also effects that we will handle pointer event
	// of the grabbed pointer
	dndSrc_.source = {appContext(), std::move(src)};
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

xcb_window_t X11DataManager::selectionOwner(xcb_atom_t selection)
{
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

void X11DataManager::unregisterDataOffer(const X11DataOffer& offer)
{
	auto end = std::remove(dndOffers_.begin(), dndOffers_.end(), &offer);
	if(dndOffers_.end() == end) {
		dlg_warn("invalid offer {} given", (void*) &offer);
		return;
	}

	dndOffers_.erase(end, dndOffers_.end());
}

xcb_connection_t& X11DataManager::xConnection() const
{
	return appContext().xConnection();
}

xcb_window_t X11DataManager::xDummyWindow() const
{
	return appContext().xDummyWindow();
}

const x11::Atoms& X11DataManager::atoms() const
{
	return appContext().atoms();
}

void X11DataManager::xdndSendEnter()
{
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
		if(targets.size() > 0) clientev.data.data32[2] = targets[0];
		if(targets.size() > 1) clientev.data.data32[3] = targets[1];
		if(targets.size() > 2) clientev.data.data32[4] = targets[2];
	}

	auto eventPtr = reinterpret_cast<const char*>(&clientev);
	xcb_send_event(&xConnection(), 0, dndSrc_.targetWindow, 0, eventPtr);
}

void X11DataManager::xdndSendLeave()
{
	xcb_client_message_event_t clientev {};
	clientev.window = dndSrc_.targetWindow;
	clientev.format = 32;
	clientev.response_type = XCB_CLIENT_MESSAGE;
	clientev.type = atoms().xdndLeave;
	clientev.data.data32[0] = dndSrc_.sourceWindow;

	auto eventPtr = reinterpret_cast<const char*>(&clientev);
	xcb_send_event(&xConnection(), 0, dndSrc_.targetWindow, 0, eventPtr);
}

void X11DataManager::xdndSendPosition(nytl::Vec2i rootPos)
{
	xcb_client_message_event_t clientev {};
	clientev.window = dndSrc_.targetWindow;
	clientev.format = 32;
	clientev.response_type = XCB_CLIENT_MESSAGE;
	clientev.type = atoms().xdndPosition;
	clientev.data.data32[0] = dndSrc_.sourceWindow;
	clientev.data.data32[1] = 0u; // reserved
	clientev.data.data32[2] = (rootPos[0] << 16) | rootPos[1];
	clientev.data.data32[3] = XCB_CURRENT_TIME;
	clientev.data.data32[4] = atoms().xdndActionCopy;

	auto eventPtr = reinterpret_cast<const char*>(&clientev);
	xcb_send_event(&xConnection(), 0, dndSrc_.targetWindow, 0, eventPtr);
}

} // namespace ny
