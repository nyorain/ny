// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/input.hpp>
#include <ny/x11/bufferSurface.hpp>
#include <ny/x11/dataExchange.hpp>

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
#include <X11/extensions/XInput2.h>

// undefine the worst Xlib macros
#undef None
#undef GenericEvent

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>

#include <cstring>
#include <mutex>
#include <atomic>
#include <queue>

namespace ny {

struct X11AppContext::Impl {
	x11::EwmhConnection ewmhConnection;
	x11::Atoms atoms;
	X11ErrorCategory errorCategory;
	X11DataManager dataManager;

#ifdef NY_WithGl
	GlxSetup glxSetup;
	bool glxFailed;
#endif //GL
};

// AppContext
X11AppContext::X11AppContext()
{
	// TODO, make this optional, may be needed by some applications
	// Something like a threadsafe flags in X11AppContextSettings would make sense
	// we use it only in wakeupWait (if called from other thread)
	XInitThreads();

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
		xDefaultScreen_->root, 0, 0, 50, 50, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
		XCB_COPY_FROM_PARENT, 0, nullptr);
	errorCategory().checkThrow(cookie, "ny::X11AppContext: create_window for dummy window failed");

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

	for(auto& name : atomNames)
		atomCookies.push_back(xcb_intern_atom(xConnection_, 0, std::strlen(name.name), name.name));

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

	// check for xinput
	int ev, err, opcode = -1;
	if(::XQueryExtension(xDisplay_, "XInputExtension", &opcode, &ev, &err)) {
		int major = 2, minor = 2;
		auto res = ::XIQueryVersion(xDisplay_, &major, &minor);
		if(res == BadRequest) {
			dlg_debug("XI2 version 2.2 not supported");
		} else if(res != Success) {
			dlg_warn("Unexpected failure to query XI extensions");
		} else {
			xiOpcode_ = opcode;
		}
	} else {
		dlg_debug("XInput not avilable");
	}

	// input
	keyboardContext_ = std::make_unique<X11KeyboardContext>(*this);
	mouseContext_ = std::make_unique<X11MouseContext>(*this);

	// data manager
	impl_->dataManager = {*this};
}

X11AppContext::~X11AppContext()
{
	if(next_) {
		free(next_);
	}
	if(xDisplay_) {
		::XFlush(&xDisplay());
	}

	xcb_ewmh_connection_wipe(&ewmhConnection());
	impl_.reset();

	if(xDummyWindow_) {
		xcb_destroy_window(xConnection_, xDummyWindow_);
	}

	if(xDisplay_) {
		::XCloseDisplay(&xDisplay());
	}
}

WindowContextPtr X11AppContext::createWindowContext(const WindowSettings& settings)
{
	X11WindowSettings x11Settings;
	const auto* ws = dynamic_cast<const X11WindowSettings*>(&settings);

	if(ws) x11Settings = *ws;
	else x11Settings.WindowSettings::operator=(settings);

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

MouseContext* X11AppContext::mouseContext()
{
	return mouseContext_.get();
}

KeyboardContext* X11AppContext::keyboardContext()
{
	return keyboardContext_.get();
}

bool X11AppContext::pollEvents()
{
	if(!checkError()) {
		return false;
	}

	deferred.execute();
	while(true) {
		xcb_flush(&xConnection());
		xcb_generic_event_t* event {};
		if(next_) {
			event = next_;
			next_ = nullptr;
		} else if(!(event = xcb_poll_for_event(xConnection_))) {
			break;
		}

		next_ = static_cast<x11::GenericEvent*>(xcb_poll_for_event(xConnection_));
		processEvent(static_cast<x11::GenericEvent&>(*event), next_);
		free(event);
	}

	xcb_flush(&xConnection());
	deferred.execute();
	return checkError();
}

bool X11AppContext::waitEvents()
{
	if(!checkError()) {
		return false;
	}

	deferred.execute();
	xcb_flush(&xConnection());

	xcb_generic_event_t* event;
	if(!(event = xcb_wait_for_event(xConnection_))) {
		dlg_warn("waitEvents: xcb_wait_for_event: I/O error");
		return checkError();
	}

	while(event) {
		xcb_flush(&xConnection());
		next_ = static_cast<x11::GenericEvent*>(xcb_poll_for_event(xConnection_));
		processEvent(static_cast<x11::GenericEvent&>(*event), next_);
		free(event);
		event = next_;
	}

	xcb_flush(&xConnection());
	deferred.execute();
	return checkError();
}

void X11AppContext::wakeupWait()
{
	// TODO: only send if currently blocking?
	// TODO: just use eventfd and custom poll instead?
	xcb_client_message_event_t dummyEvent {};
	dummyEvent.type = XCB_CLIENT_MESSAGE;
	auto eventData = reinterpret_cast<const char*>(&dummyEvent);
	xcb_send_event(xConnection_, 0, xDummyWindow(), 0, eventData);
	xcb_flush(xConnection_);
}

bool X11AppContext::clipboard(std::unique_ptr<DataSource>&& dataSource)
{
	return impl_->dataManager.clipboard(std::move(dataSource));
}

DataOffer* X11AppContext::clipboard()
{
	return impl_->dataManager.clipboard();
}

bool X11AppContext::startDragDrop(std::unique_ptr<DataSource>&& dataSource)
{
	return impl_->dataManager.startDragDrop(std::move(dataSource));
}

std::vector<const char*> X11AppContext::vulkanExtensions() const
{
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif
}

GlSetup* X11AppContext::glSetup() const
{
	#ifdef NY_WithGl
		return glxSetup();
	#else
		return nullptr;
	#endif
}

GlxSetup* X11AppContext::glxSetup() const
{
	#ifdef NY_WithGl
		if(impl_->glxFailed) return nullptr;

		if(!impl_->glxSetup.valid()) {
			try {
				impl_->glxSetup = {*this};
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

void X11AppContext::registerContext(xcb_window_t w, X11WindowContext& c)
{
	contexts_[w] = &c;
}

void X11AppContext::unregisterContext(xcb_window_t w)
{
	contexts_.erase(w);
}

X11WindowContext* X11AppContext::windowContext(xcb_window_t win)
{
	if(contexts_.find(win) != contexts_.end())
		return contexts_[win];

	return nullptr;
}

bool X11AppContext::checkError()
{
	if(error_) {
		return false;
	}

	auto err = xcb_connection_has_error(xConnection_);
	if(err) {
		const char* name = "<unknown error>";
		#define NY_CASE(x) case x: name = #x; break;
		switch(err) {
			NY_CASE(XCB_CONN_ERROR);
			NY_CASE(XCB_CONN_CLOSED_EXT_NOTSUPPORTED);
			NY_CASE(XCB_CONN_CLOSED_REQ_LEN_EXCEED);
			NY_CASE(XCB_CONN_CLOSED_PARSE_ERR);
			NY_CASE(XCB_CONN_CLOSED_INVALID_SCREEN);
			default: break;
		}
		#undef NY_CASE

		auto msg = dlg::format("X11AppContext: xcb_connection has "
			"critical error:\n\t'{}' (code {})", name, err);
		dlg_error(msg);
		error_ = true;
		return false;
	}

	return true;
}

xcb_atom_t X11AppContext::atom(const std::string& name)
{
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

void X11AppContext::bell()
{
	// TODO: rather random value here. accept (defaulted) parameter?
	xcb_bell(xConnection_, 100);
}

void X11AppContext::processEvent(const x11::GenericEvent& ev, const x11::GenericEvent* next)
{
	X11EventData eventData {ev};

	// TODO: optimize deferred operations, code duplication atm
	//  + maybe just use this defered method in X11WindowContext::refresh
	//  instead of real event

	// TODO: make windowContext handle events to remove friend decl

	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
		case XCB_EXPOSE: {
			auto& expose = reinterpret_cast<const xcb_expose_event_t&>(ev);
			auto wc = windowContext(expose.window);
			if(wc) {
				if(!wc->drawEventFlag_) {
					wc->drawEventFlag_ = true;
					deferred.add([wc, eventData](){
						DrawEvent de;
						de.eventData = &eventData;
						wc->listener().draw(de);
						wc->drawEventFlag_ = false;
					}, wc);
				}
			}
			break;
		}

		case XCB_MAP_NOTIFY: {
			auto& map = reinterpret_cast<const xcb_map_notify_event_t&>(ev);
			auto wc = windowContext(map.event);
			if(wc) {
				if(!wc->drawEventFlag_) {
					wc->drawEventFlag_ = true;
					deferred.add([wc, eventData](){
						DrawEvent de;
						de.eventData = &eventData;
						wc->listener().draw(de);
						wc->drawEventFlag_ = false;
					}, wc);
				}
			}
			break;
		}

		case XCB_REPARENT_NOTIFY: {
			auto& reparent = reinterpret_cast<const xcb_reparent_notify_event_t&>(ev);
			auto wc = windowContext(reparent.window);
			if(wc) {
				wc->reparentEvent();
			}
			break;
		}

		case XCB_CONFIGURE_NOTIFY: {
			auto& configure = reinterpret_cast<const xcb_configure_notify_event_t&>(ev);

			auto nsize = nytl::Vec2ui{configure.width, configure.height};
			auto wc = windowContext(configure.window);

			if(wc && nsize != wc->size()) {
				wc->updateSize(nsize);
				wc->reloadStates();
				if(!wc->resizeEventFlag_) {
					wc->resizeEventFlag_ = true;
					deferred.add([wc, eventData](){
						SizeEvent se;
						se.size = wc->size();
						se.eventData = &eventData;
						wc->listener().resize(se);
						wc->resizeEventFlag_ = false;
					}, wc);
				}
			}

			break;
		}

		case XCB_CLIENT_MESSAGE: {
			auto& client = reinterpret_cast<const xcb_client_message_event_t&>(ev);
			auto protocol = static_cast<unsigned int>(client.data.data32[0]);

			auto wc = windowContext(client.window);
			if(protocol == atoms().wmDeleteWindow && wc) {
				CloseEvent ce;
				ce.eventData = &eventData;
				wc->listener().close(ce);
			} else if(protocol == ewmhConnection()._NET_WM_PING) {
				xcb_ewmh_send_wm_ping(&ewmhConnection(), xDefaultScreen().root,
					client.data.data32[1]);
			}

			break;
		}

	case 0u: {
			int code = reinterpret_cast<const xcb_generic_error_t&>(ev).error_code;
			auto errorMsg = x11::errorMessage(xDisplay(), code);
			dlg_warn("retrieved error code {}, {}", code, errorMsg);

			break;
		}

		default: break;
	}

	if(impl_->dataManager.processEvent(ev)) return;
	if(keyboardContext_->processEvent(ev, next)) return;
	if(mouseContext_->processEvent(ev)) return;

	// touch events
	auto& gev = reinterpret_cast<const xcb_ge_generic_event_t&>(ev);
	if(xiOpcode_ && gev.response_type == XCB_GE_GENERIC &&
			gev.extension == xiOpcode_) {

		// Taken from xcb xinput.h
		struct xcb_input_group_info_t {
			uint8_t base;
			uint8_t latched;
			uint8_t locked;
			uint8_t effective;
		};

		struct xcb_input_modifier_info_t {
			uint32_t base;
			uint32_t latched;
			uint32_t locked;
			uint32_t effective;
		};

		struct xcb_input_touch_begin_event_t {
			uint8_t response_type;
			uint8_t extension;
			uint16_t sequence;
			uint32_t length;
			uint16_t event_type;
			uint16_t deviceid;
			xcb_timestamp_t time;
			uint32_t detail;
			xcb_window_t root;
			xcb_window_t event;
			xcb_window_t child;
			uint32_t full_sequence;
			int32_t root_x;
			int32_t root_y;
			int32_t event_x;
			int32_t event_y;
			uint16_t buttons_len;
			uint16_t valuators_len;
			int16_t sourceid;
			uint8_t pad0[2];
			uint32_t flags;
			xcb_input_modifier_info_t mods;
			xcb_input_group_info_t  group;
		};

		auto& tev = reinterpret_cast<const xcb_input_touch_begin_event_t&>(gev);
		auto wc = windowContext(tev.event);
		if(!wc) {
			return;
		}

		static constexpr auto fp16 = 65536.f;
		auto pos = nytl::Vec2f {tev.event_x / fp16, tev.event_y / fp16};
		auto detail = static_cast<unsigned>(tev.detail);
		auto data = X11EventData {ev};
		switch(gev.event_type) {
			case XI_TouchBegin:
				wc->listener().touchBegin({&data, pos, detail});
				return;
			case XI_TouchUpdate:
				wc->listener().touchUpdate({&data, pos, detail});
				return;
			case XI_TouchEnd:
				wc->listener().touchEnd({&data, pos, detail});
				return;
		}
	}
}

x11::EwmhConnection& X11AppContext::ewmhConnection() const { return impl_->ewmhConnection; }
X11ErrorCategory& X11AppContext::errorCategory() const { return impl_->errorCategory; }
const x11::Atoms& X11AppContext::atoms() const { return impl_->atoms; }
X11DataManager& X11AppContext::dataManager() const { return impl_->dataManager; }

} // namespace ny
