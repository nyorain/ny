// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/input.hpp>
#include <ny/x11/bufferSurface.hpp>
#include <ny/x11/dataExchange.hpp>
#include <ny/common/copy.hpp>
#include <ny/common/connectionList.hpp>

#include <ny/common/unix.hpp>
#include <ny/dataExchange.hpp>

#ifdef NY_WithVulkan
	#define VK_USE_PLATFORM_XCB_KHR
	#include <ny/x11/vulkan.hpp>
	#include <vulkan/vulkan.h>
#endif //Vulkan

#ifdef NY_WithGl
	#include <ny/x11/glx.hpp>
#endif //GL

#include <nytl/scope.hpp>
#include <nytl/vecOps.hpp>
#include <dlg/dlg.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>

// undefine the worst Xlib macros
#undef None
#undef GenericEvent

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/present.h>
#include <xcb/xinput.h>
#include <xcb/shm.h>

#include <poll.h>
#include <unistd.h>
#include <cstring>
#include <mutex>
#include <atomic>
#include <queue>

namespace ny {
namespace {

// Like poll but does not return on signals.
int noSigPoll(pollfd& fds, nfds_t nfds, int timeout = -1) {
	while(true) {
		auto ret = poll(&fds, nfds, timeout);
		if(ret != -1 || errno != EINTR) return ret;
	}
}

// Listener entry to implement custom fd polling callbacks in WaylandAppContext.
struct ListenerEntry {
	int fd {};
	unsigned int events {};
	std::function<bool(int fd, unsigned int events)> callback;
};

} // anon namespace

struct X11AppContext::Impl {
	x11::EwmhConnection ewmhConnection;
	x11::Atoms atoms;
	X11ErrorCategory errorCategory;
	X11DataManager dataManager;
	ConnectionList<ListenerEntry> fdCallbacks;

#ifdef NY_WithGl
	GlxSetup glxSetup;
	bool glxFailed;
#endif //GL
};

// AppContext
X11AppContext::X11AppContext() {
	// TODO, make this optional, may be requested by some applications
	// for additional operations
	// ::XInitThreads();
	impl_ = std::make_unique<Impl>();

	xDisplay_ = ::XOpenDisplay(nullptr);
	if(!xDisplay_) {
		throw std::runtime_error("ny::X11AppContext: could not connect to X Server");
	}

	xDefaultScreenNumber_ = ::XDefaultScreen(xDisplay_);

	xConnection_ = ::XGetXCBConnection(xDisplay_);
	if(!xConnection_ || xcb_connection_has_error(xConnection_)) {
		throw std::runtime_error("ny::X11AppContext: unable to get xcb connection");
	}

	impl_->errorCategory = {*xDisplay_, *xConnection_};
	auto ewmhCookie = xcb_ewmh_init_atoms(&xConnection(), &ewmhConnection());

	// query server information
	auto iter = xcb_setup_roots_iterator(xcb_get_setup(&xConnection()));
	for(auto i = 0; iter.rem; ++i, xcb_screen_next(&iter)) {
		if(i == xDefaultScreenNumber_) {
			xDefaultScreen_ = iter.data;
			break;
		}
	}

	// This must be called because xcb is used to access the event queue
	::XSetEventQueueOwner(xDisplay_, XCBOwnsEventQueue);

	// Generate an x dummy window that can e.g. be used for selections
	// This window remains invisible, i.e. it is not begin mapped
	xDummyWindow_ = xcb_generate_id(xConnection_);
	auto cookie = xcb_create_window_checked(xConnection_, XCB_COPY_FROM_PARENT, xDummyWindow_,
		xDefaultScreen_->root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		XCB_COPY_FROM_PARENT, 0, nullptr);
	errorCategory().checkThrow(cookie, "ny::X11AppContext: create_window for dummy window failed");

	auto presentid = xcb_generate_id(&xConnection());
	uint32_t presentMask =
		XCB_PRESENT_EVENT_MASK_CONFIGURE_NOTIFY |
		XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY |
		XCB_PRESENT_EVENT_MASK_IDLE_NOTIFY;
	xcb_present_select_input(&xConnection(), presentid,
		xDummyWindow(), presentMask);

	// dummy pixmap for dummy window
	xDummyPixmap_ = xcb_generate_id(xConnection_);
	auto pcookie = xcb_create_pixmap_checked(xConnection_, xDefaultScreen_->root_depth,
		xDummyPixmap_, xDummyWindow_, 1, 1);
	errorCategory().checkThrow(pcookie, "ny::X11AppContext: failed to create dummy pixmap");

	// empty region
	// xcb_rectangle_t rect = {};
	// rect.width = 1;
	// rect.height = 1;
	// xEmptyRegion_ = xcb_generate_id(xConnection_);
	// auto rcookie = xcb_xfixes_create_region_checked(xConnection_, xEmptyRegion_, 1, &rect);
	// errorCategory().checkThrow(rcookie, "ny::X11AppContext: failed to create empty region");

	// Load all default required atoms
	auto& atoms = impl_->atoms;
	struct {
		xcb_atom_t& atom;
		const char* name;
	} atomNames[] = {
		{atoms.xdndEnter, "XdndEnter"},
		{atoms.xdndPosition, "XdndPosition"},
		{atoms.xdndStatus, "XdndStatus"},
		{atoms.xdndTypeList, "XdndTypeList"},
		{atoms.xdndActionCopy, "XdndActionCopy"},
		{atoms.xdndActionMove, "XdndActionMove"},
		{atoms.xdndActionAsk, "XdndActionAsk"},
		{atoms.xdndActionLink, "XdndActionLink"},
		{atoms.xdndDrop, "XdndDrop"},
		{atoms.xdndLeave, "XdndLeave"},
		{atoms.xdndFinished, "XdndFinished"},
		{atoms.xdndSelection, "XdndSelection"},
		{atoms.xdndProxy, "XdndProxy"},
		{atoms.xdndAware, "XdndAware"},

		{atoms.clipboard, "CLIPBOARD"},
		{atoms.targets, "TARGETS"},
		{atoms.text, "TEXT"},
		{atoms.utf8string, "UTF8_STRING"},
		{atoms.fileName, "FILE_NAME"},

		{atoms.wmDeleteWindow, "WM_DELETE_WINDOW"},
		{atoms.motifWmHints, "_MOTIF_WM_HINTS"},

		{atoms.mime.textPlain, "text/plain"},
		{atoms.mime.textPlainUtf8, "text/plain;charset=utf8"},
		{atoms.mime.textUriList, "text/uri-list"},

		{atoms.mime.imageData, "image/x-ny-data"},
		{atoms.mime.raw, "application/octet-stream"}
	};

	auto length = sizeof(atomNames) / sizeof(atomNames[0]);

	std::vector<xcb_intern_atom_cookie_t> atomCookies;
	atomCookies.reserve(length);

	for(auto& name : atomNames) {
		auto len = std::strlen(name.name);
		auto atom = xcb_intern_atom(xConnection_, 0, len, name.name);
		atomCookies.push_back(atom);
	}

	for(auto i = 0u; i < atomCookies.size(); ++i) {
		xcb_generic_error_t* error {};
		auto reply = xcb_intern_atom_reply(xConnection_, atomCookies[i], &error);
		if(reply) {
			atomNames[i].atom = reply->atom;
			free(reply);
			continue;
		} else if(error) {
			auto msg = x11::errorMessage(xDisplay(), error->error_code);
			dlg_warn("Failed to load atom {} : {}", atomNames[i].name, msg);
			free(error);
		}
	}

	// ewmh atoms
	xcb_ewmh_init_atoms_replies(&ewmhConnection(), ewmhCookie, nullptr);

	// check _NET_WM_SUPPORTED
	auto prop = x11::readProperty(xConnection(),
		ewmhConnection()._NET_SUPPORTED, xDefaultScreen().root);
	if(prop.data.empty()) {
		dlg_info("ewmh not supported. Some features may not work");
	} else {
		dlg_assert(prop.format == 32);
		dlg_assert(prop.type == XCB_ATOM_ATOM);
		auto atoms = nytl::Span<xcb_atom_t>{
			reinterpret_cast<xcb_atom_t*>(prop.data.data()),
			prop.data.size() / 4
		};

		auto supported = [&](xcb_atom_t atom) {
			return std::find(atoms.begin(), atoms.end(), atom) != atoms.end();
		};

		if(supported(ewmhConnection()._NET_WM_MOVERESIZE)) {
			ewmhWindowCaps_ |= WindowCapability::beginMove;
			ewmhWindowCaps_ |= WindowCapability::beginResize;
		}

		if(supported(ewmhConnection()._NET_WM_STATE)) {
			auto max =
				supported(ewmhConnection()._NET_WM_STATE_MAXIMIZED_HORZ) &&
				supported(ewmhConnection()._NET_WM_STATE_MAXIMIZED_VERT);
			if(max) {
				ewmhWindowCaps_ |= WindowCapability::maximize;
			}

			if(supported(ewmhConnection()._NET_WM_STATE_FULLSCREEN)) {
				ewmhWindowCaps_ |= WindowCapability::fullscreen;
			}
		}
	}

	// NOTE: not really sure about those versions, kinda strict atm
	// since that's what other libraries seem to do. We might be fine
	// with lower versions of the extensions

	// check for xinput support
	auto ext = xcb_get_extension_data(&xConnection(), &xcb_input_id);
	if(ext && ext->present) {
		xinputOpcode_ = ext->major_opcode;
		auto cookie = xcb_input_xi_query_version(&xConnection(), 2, 0);
		auto reply = xcb_input_xi_query_version_reply(&xConnection(),
			cookie, nullptr);
		if(!reply || reply->major_version < 2) {
			dlg_warn("xinput version too low: {}.{}",
				reply->major_version, reply->minor_version);
			xinputOpcode_ = {};
		}
		free(reply);
	}
	if(!xinputOpcode_) {
		dlg_warn("xinput not available: touch input not supported");
	}

	// check for present extension support
	ext = xcb_get_extension_data(&xConnection(), &xcb_present_id);
	if(ext && ext->present) {
		presentOpcode_ = ext->major_opcode;
		auto cookie = xcb_present_query_version(&xConnection(), 1, 2);
		auto reply = xcb_present_query_version_reply(&xConnection(),
			cookie, nullptr);
		if(!reply || reply->major_version < 1 || reply->minor_version < 2) {
			presentOpcode_ = {};
			dlg_warn("x11 server does not support present extension");
		}
		free(reply);
	}

	// check for shm extension support
	{
		auto cookie = xcb_shm_query_version(&xConnection());
		auto reply = xcb_shm_query_version_reply(&xConnection(), cookie, nullptr);
		if(reply && reply->shared_pixmaps && reply->major_version >= 1 &&
				reply->minor_version >= 2) {
			shmExt_ = true;
		} else {
			dlg_warn("x11 server does not support shm extension");
		}
		free(reply);
	}

	// input
	keyboardContext_ = std::make_unique<X11KeyboardContext>(*this);
	mouseContext_ = std::make_unique<X11MouseContext>(*this);

	// data manager
	impl_->dataManager = {*this};
}

X11AppContext::~X11AppContext() {
	if(next_) {
		free(next_);
	}
	if(xDisplay_) {
		::XFlush(&xDisplay());
	}

	xcb_ewmh_connection_wipe(&ewmhConnection());
	impl_->dataManager = {}; // destroy dnd window
	impl_.reset();

	if(xDummyWindow_) {
		xcb_destroy_window(xConnection_, xDummyWindow_);
	}

	if(xDisplay_) {
		::XCloseDisplay(&xDisplay());
	}
}

WindowContextPtr X11AppContext::createWindowContext(const WindowSettings& settings) {
	X11WindowSettings x11Settings;
	const auto* ws = dynamic_cast<const X11WindowSettings*>(&settings);
	if(ws) {
		x11Settings = *ws;
	} else {
		x11Settings.WindowSettings::operator=(settings);
	}

	// type
	if(settings.surface == SurfaceType::vulkan) {
		#ifdef NY_WithVulkan
			return std::make_unique<X11VulkanWindowContext>(*this, x11Settings);
		#else
			throw std::logic_error("ny::X11AppContext::createWindowContext: "
				"ny built without vulkan support, cannot create the requested vulkan surface");
		#endif
	} else if(settings.surface == SurfaceType::gl) {
		#ifdef NY_WithGl
			static constexpr auto glxFailed = "ny::X11AppContext::createWindowContext: "
				"failed to init glx, cannot create the requested gl surface";

			if(!glxSetup()) throw std::runtime_error(glxFailed);
			return std::make_unique<GlxWindowContext>(*this, *glxSetup(), x11Settings);
		#else
			throw std::logic_error("ny::X11AppContext::createWindowContext: "
				"ny built without gl/glx support, cannot create the requested gl surface");
		#endif
	} else if(settings.surface == SurfaceType::buffer) {
		return std::make_unique<X11BufferWindowContext>(*this, x11Settings);
	}

	return std::make_unique<X11WindowContext>(*this, x11Settings);
}

MouseContext* X11AppContext::mouseContext() {
	return mouseContext_.get();
}

KeyboardContext* X11AppContext::keyboardContext() {
	return keyboardContext_.get();
}

bool X11AppContext::dispatchPending() {
	// dispatch all readable events
	auto i = 0u;
	while(true) {
		xcb_generic_event_t* event;
		if(next_) {
			event = static_cast<xcb_generic_event_t*>(next_);
			next_ = {};
		} else if(!(event = xcb_poll_for_event(xConnection_))) {
			break;
		}

		// NOTE: only re-entrant under the assumption that next_ won't
		// be used by processEvent (or anything it calls) *after* it
		// gives control to a callback outside ny, since that might
		// call something ending up here again, eventually freeing next_.
		// A solution would be to copy next_ locally before passing it
		// to processEvent but we can't copy generic events since we don't
		// know their full size.
		// TODO: Alternatively we could abolish the whole next_ mechanism, it's
		// only needed (at the time of writing) in x11/input.cpp to
		// check whether a key press is repeated; we could simply add
		// a xcb_poll_for_event there to check but that feels hacky as well
		// (but probably still better than this!)
		next_ = xcb_poll_for_event(xConnection_);
		processEvent(event, next_);
		free(event);
		++i;
	}

	return i > 0;
}

bool X11AppContext::poll(bool wait) {
	auto res = false;
	eventfd_.reset();

	std::vector<nytl::ConnectionID> ids;
	std::vector<pollfd> fds;
	ids.reserve(impl_->fdCallbacks.items.size());
	fds.reserve(impl_->fdCallbacks.items.size() + 2);

	// custom fds
	for(auto& fdc : impl_->fdCallbacks.items) {
		fds.push_back({fdc.fd, static_cast<short>(fdc.events), 0u});
		ids.push_back({fdc.clID_});
	}

	// xcb connection
	fds.emplace_back();
	fds.back().events = POLLIN;
	fds.back().fd = xcb_get_file_descriptor(&xConnection());

	// eventfd at last index
	// only needed when waiting, otherwise we won't ever block at all
	if(wait) {
		fds.emplace_back();
		fds.back().events = POLLIN;
		fds.back().fd = eventfd_.fd();
	}

	// poll
	auto timeout = wait ? -1 : 0;
	auto ret = noSigPoll(*fds.data(), fds.size(), timeout);
	if(ret < 0) {
		dlg_info("poll failed: {}", std::strerror(errno));
		return false;
	} else if(ret == 0) { // timeout, no events
		return false;
	}

	// check eventfd
	if(wait) {
		if(fds.back().revents & POLLIN) {
			dlg_assert(eventfd_.reset());
			return true;
		}
		fds.pop_back();
	}

	// deliver events to custom fds
	// check which fd callbacks have revents, find and trigger them
	// fds.size() > ids.size() always true
	for(auto i = 0u; i < ids.size(); ++i) {
		if(!fds[i].revents) {
			continue;
		}

		res = true;
		auto& items = impl_->fdCallbacks.items;
		auto it = std::find_if(items.begin(), items.end(),
			[&](auto& cb){ return cb.clID_.get() == ids[i].get(); });
		if(it == items.end()) {
			// in this case it was erased by a previous callback
			continue;
		}

		if(!it->callback(fds[i].fd, fds[i].revents)) {
			// refresh iterator. fdCallbacks.items might have
			// been changed
			auto it = std::find_if(items.begin(), items.end(),
				[&](auto& cb){ return cb.clID_.get() == ids[i].get(); });
			if(it != items.end()) {
				items.erase(it);
			}
			// otherwise returned false was already actively disconnected
		}
	}

	if(fds.back().revents & POLLIN) {
		res |= dispatchPending();
	}

	return res;
}

// TODO: not sure why dispatchPending is needed before poll...
// seems like xcb_poll_for_events might even have events
// when POLLIN doesn't return any events

// TODO: xcb_flush may block until data can be written
// we could avoid that by polling for POLLOUT and only call
// xcb_flush in that case... worth it?
// wayland backend already roughly does it that way
void X11AppContext::pollEvents() {
	checkError();
	// execDeferred();
	xcb_flush(&xConnection());
	dispatchPending();
	poll(false);
	execDeferred();
	checkError();
}

void X11AppContext::waitEvents() {
	checkError();

	// execDeferred();
	xcb_flush(&xConnection());

	if(!dispatchPending() && !poll(false) && defer_.empty()) {
		// no pending events processed, wait for one
		poll(true);
	}

	execDeferred();
	checkError();
}

void X11AppContext::wakeupWait() {
	eventfd_.signal();
}

bool X11AppContext::clipboard(std::unique_ptr<DataSource>&& dataSource) {
	return impl_->dataManager.clipboard(std::move(dataSource));
}

DataOffer* X11AppContext::clipboard() {
	return impl_->dataManager.clipboard();
}

bool X11AppContext::dragDrop(const EventData* ev,
		std::unique_ptr<DataSource>&& dataSource) {
	return impl_->dataManager.startDragDrop(ev, std::move(dataSource));
}

std::vector<const char*> X11AppContext::vulkanExtensions() const {
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif
}

GlSetup* X11AppContext::glSetup() const {
	#ifdef NY_WithGl
		return glxSetup();
	#else
		return nullptr;
	#endif
}

GlxSetup* X11AppContext::glxSetup() const {
	#ifdef NY_WithGl
		if(impl_->glxFailed) {
			return nullptr;
		}

		if(!impl_->glxSetup.valid()) {
			try {
				impl_->glxSetup = {*this, unsigned(xDefaultScreenNumber())};
			} catch(const std::exception& error) {
				dlg_warn("initialization failed: {}", error.what());
				impl_->glxFailed = true;
				impl_->glxSetup = {};
				return nullptr;
			}
		}

		return &impl_->glxSetup;

	#else
		return nullptr;

	#endif
}

unsigned X11AppContext::execDeferred() {
	auto count = 0u;
	while(!defer_.empty()) {
		auto current = defer_.front();
		defer_.pop_front();
		current.func(*current.wc);
		++count;
	}

	return count;
}

void X11AppContext::defer(X11WindowContext& wc, DeferFunc f) {
	defer_.push_back({f, &wc});
}

void X11AppContext::registerContext(xcb_window_t w, X11WindowContext& c) {
	contexts_[w] = &c;
}

void X11AppContext::destroyed(const X11WindowContext& wc) {
	defer_.erase(std::remove_if(defer_.begin(), defer_.end(),
		[&](auto& o){ return &wc == o.wc; }), defer_.end());

	if(auto xwin = wc.xWindow(); xwin) {
		contexts_.erase(xwin);
	}

	dataManager().destroyed(wc);
	keyboardContext_->destroyed(wc);
	mouseContext_->destroyed(wc);
}

X11WindowContext* X11AppContext::windowContext(xcb_window_t win) {
	if(contexts_.find(win) != contexts_.end()) {
		return contexts_[win];
	}

	return nullptr;
}

void X11AppContext::checkError() {
	auto err = xcb_connection_has_error(xConnection_);
	if(!err) {
		return;
	}

	const char* name = "<unknown error>";
	#define ERR_CASE(x) case x: name = #x; break;
	switch(err) {
		ERR_CASE(XCB_CONN_ERROR);
		ERR_CASE(XCB_CONN_CLOSED_EXT_NOTSUPPORTED);
		ERR_CASE(XCB_CONN_CLOSED_REQ_LEN_EXCEED);
		ERR_CASE(XCB_CONN_CLOSED_PARSE_ERR);
		ERR_CASE(XCB_CONN_CLOSED_INVALID_SCREEN);
		default: break;
	}
	#undef NY_CASE

	auto msg = dlg::format("X11AppContext: xcb_connection has "
		"critical error:\n\t'{}' (code {})", name, err);
	throw BackendError(msg);
}

xcb_atom_t X11AppContext::atom(const std::string& name) {
	auto it = additionalAtoms_.find(name);
	if(it == additionalAtoms_.end()) {
		auto cookie = xcb_intern_atom(xConnection_, 0, name.size(), name.c_str());
		xcb_generic_error_t* error;
		auto reply = xcb_intern_atom_reply(xConnection_, cookie, &error);
		if(error) {
			auto msg = x11::errorMessage(xDisplay(), error->error_code);
			dlg_warn("failed to retrieve x11 atom {}: {}", name, msg);
			free(error);
			return 0u;
		}

		it = additionalAtoms_.insert({name, reply->atom}).first;
		free(reply);
	}

	return it->second;
}

void X11AppContext::bell(unsigned val) {
	xcb_bell(xConnection_, val);
}


void X11AppContext::processEvent(const void* pev, const void* pnext) {
	dlg_assert(pev);
	auto& ev = *static_cast<const xcb_generic_event_t*>(pev);
	X11EventData eventData(ev);

	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
	case XCB_EXPOSE: {
		auto expose = copyu<xcb_expose_event_t>(ev);
		if(auto wc = windowContext(expose.window); wc) {
			wc->scheduleRedraw();
		}
		break;
	} case XCB_MAP_NOTIFY: {
		auto map = copyu<xcb_map_notify_event_t>(ev);
		if(auto wc = windowContext(map.event); wc) {
			wc->scheduleRedraw();
		}
		break;
	} case XCB_REPARENT_NOTIFY: {
		auto reparent = copyu<xcb_reparent_notify_event_t>(ev);
		auto wc = windowContext(reparent.window);
		if(wc) {
			wc->reparentEvent();
		}
		break;
	} case XCB_CONFIGURE_NOTIFY: {
		auto configure = copyu<xcb_configure_notify_event_t>(ev);
		if(auto wc = windowContext(configure.window); wc) {
			wc->resizeEvent({configure.width, configure.height}, eventData);
		}
		break;
	} case XCB_CLIENT_MESSAGE: {
		auto client = copyu<xcb_client_message_event_t>(ev);
		auto protocol = static_cast<unsigned int>(client.data.data32[0]);

		auto wc = windowContext(client.window);
		if(protocol == atoms().wmDeleteWindow && wc) {
			CloseEvent ce;
			ce.eventData = &eventData;
			wc->listener().close(ce);
		} else if(protocol == ewmhConnection()._NET_WM_PING) {
			xcb_ewmh_send_wm_ping(&ewmhConnection(),
				xDefaultScreen().root, client.data.data32[1]);
		}

		break;
	} case 0u: {
		int code = copyu<xcb_generic_error_t>(ev).error_code;
		auto errorMsg = x11::errorMessage(xDisplay(), code);
		dlg_warn("retrieved error code {}, {}", code, errorMsg);

		break;
	} default:
		break;
	}

	if(impl_->dataManager.processEvent(pev) ||
			keyboardContext_->processEvent(pev, pnext) ||
			mouseContext_->processEvent(pev)) {
		return;
	}

	// present event
	auto gev = copyu<xcb_ge_generic_event_t>(ev);
	if(presentOpcode_ && gev.response_type == XCB_GE_GENERIC &&
			gev.extension == presentOpcode_) {
		switch(gev.event_type) {
			case XCB_PRESENT_COMPLETE_NOTIFY: {
				auto pev = copyu<xcb_present_complete_notify_event_t>(gev);
				if(auto wc = windowContext(pev.window); wc) {
					wc->presentCompleteEvent(pev.serial);
				}
				return;
			} case XCB_PRESENT_IDLE_NOTIFY:
			case XCB_PRESENT_CONFIGURE_NOTIFY:
				return;
		}
	}

	// touch events
	if(xinputOpcode_ && gev.response_type == XCB_GE_GENERIC &&
			gev.extension == xinputOpcode_) {
		// no matter the event type, always has the same basic layout
		auto tev = copyu<xcb_input_touch_begin_event_t>(gev);
		time(tev.time);
		auto wc = windowContext(tev.event);
		if(!wc) {
			return;
		}

		static constexpr auto fp16 = 65536.f;
		auto pos = nytl::Vec2f {tev.event_x / fp16, tev.event_y / fp16};
		auto detail = static_cast<unsigned>(tev.detail);
		auto data = X11EventData {ev};
		switch(gev.event_type) {
			case XCB_INPUT_TOUCH_BEGIN:
				wc->listener().touchBegin({&data, pos, detail});
				return;
			case XCB_INPUT_TOUCH_UPDATE:
				wc->listener().touchUpdate({&data, pos, detail});
				return;
			case XCB_INPUT_TOUCH_END:
				wc->listener().touchEnd({&data, pos, detail});
				return;
		}
	}
}

nytl::Connection X11AppContext::fdCallback(int fd, unsigned int events,
		FdCallbackFunc func) {
	return impl_->fdCallbacks.add({fd, events, std::move(func)});
}

x11::EwmhConnection& X11AppContext::ewmhConnection() const { return impl_->ewmhConnection; }
X11ErrorCategory& X11AppContext::errorCategory() const { return impl_->errorCategory; }
const x11::Atoms& X11AppContext::atoms() const { return impl_->atoms; }
X11DataManager& X11AppContext::dataManager() const { return impl_->dataManager; }

} // namespace ny
