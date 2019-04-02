// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/dataExchange.hpp>
#include <ny/wayland/util.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/asyncRequest.hpp>
#include <dlg/dlg.hpp>

#include <nytl/scope.hpp>
#include <nytl/tmpUtil.hpp> // nytl::unused

#include <wayland-client-protocol.h>
#include <wayland-cursor.h>

#include <string_view>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <signal.h>

#include <algorithm>

namespace ny {

// WaylandDataOffer
WaylandDataOffer::WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer)
		: appContext_(&ac), wlDataOffer_(&wlDataOffer) {
	static constexpr wl_data_offer_listener listener {
		memberCallback<&WaylandDataOffer::offer>,
		memberCallback<&WaylandDataOffer::sourceActions>,
		memberCallback<&WaylandDataOffer::handleAction>
	};

	wl_data_offer_add_listener(&wlDataOffer, &listener, this);
}

WaylandDataOffer::~WaylandDataOffer() {
	destroy();
}

void WaylandDataOffer::destroy() {
	// signal all pending requests that they have failed
	for(auto& entry : pending_) {
		for(auto& listener : entry.second.listener) {
			listener({});
		}
	}

	if(wlDataOffer_) {
		auto version = wl_data_offer_get_version(wlDataOffer_);
		if(action_ != DndAction::none && !accepted_.empty() &&
				version >= WL_DATA_OFFER_FINISH_SINCE_VERSION) {
			wl_data_offer_finish(wlDataOffer_);
		}
		wl_data_offer_destroy(wlDataOffer_);
	}
}

bool WaylandDataOffer::formats(FormatsListener listener) {
	listener(formats_);
	return true;
}

bool WaylandDataOffer::data(nytl::StringParam format, DataListener listener) {
	auto fit = std::find(formats_.begin(), formats_.end(), format);
	if(fit == formats_.end()) {
		dlg_warn("unsupported format {}", format);
		return false;
	}

	auto rit = pending_.find(format.c_str());
	if(rit != pending_.end()) {
		rit->second.listener.push_back(std::move(listener));
		return true;
	}

	Request req {};
	req.listener.push_back(std::move(listener));

	// create the pipe nonblocking, to allow us differentiating
	// eof and no data avilable
	int fds[2];
	auto ret = pipe2(fds, O_CLOEXEC | O_NONBLOCK);
	if(ret < 0) {
		dlg_warn("pipe2 failed: {}", std::strerror(errno));
		return {};
	}

	wl_data_offer_receive(wlDataOffer_, format.c_str(), fds[1]);
	close(fds[1]);

	// WaylandDataOffer is non-movable so we can capture [this] here
	auto fmt = std::string(format);
	auto callback = [this, fmt](int fd, unsigned int) {
		return this->fdCallbackDnd(fd, fmt);
	};

	// also listen to POLLHUP since that might be sent when pipe is closed
	req.fdConnection = appContext_->fdCallback(fds[0],
		POLLIN | POLLHUP, callback);
	pending_.insert({format.c_str(), std::move(req)});
	return true;
}

void WaylandDataOffer::preferred(nytl::StringParam format, DndAction action) {
	if(std::find(formats_.begin(), formats_.end(), format) == formats_.end()) {
		dlg_warn("preferred format is not supported");
		format = {};
	}

	auto dndSerial = appContext().waylandDataDevice()->dndSerial();
	auto wlAction = dndActionToWayland(action);
	if(wl_data_offer_get_version(&wlDataOffer()) >=
			WL_DATA_OFFER_SET_ACTIONS_SINCE_VERSION) {
		wl_data_offer_set_actions(&wlDataOffer(), wlAction, wlAction);
	}

	auto fmtc = format.empty() ? nullptr : format.c_str();
	wl_data_offer_accept(&wlDataOffer(), dndSerial, fmtc);
	accepted_ = format;
}

void WaylandDataOffer::offer(wl_data_offer*, const char* fmt) {
	formats_.push_back(fmt);
}

void WaylandDataOffer::sourceActions(wl_data_offer*, uint32_t wlactions) {
	actions_ = waylandToDndActions(wlactions);
}

void WaylandDataOffer::handleAction(wl_data_offer*, uint32_t action) {
	// NOTE: ny currently has no way to signal the receiving about the dnd
	// action
	action_ = waylandToDndAction(action);
}

bool WaylandDataOffer::fdCallbackDnd(int fd, std::string fmt) {
	// TODO: unset finish_ on error?
	// no sure whether or not finish should be called when an error
	// ocurrs during exchange

	auto it = pending_.find(fmt.c_str());
	dlg_assert(it != pending_.end());
	auto& req = it->second;

	auto readCount = 2048u;
	while(true) {
		auto size = req.buffer.size();
		req.buffer.resize(size + readCount);
		auto ret = read(fd, req.buffer.data() + size, readCount);

		if(ret == 0) {
			// erase end data
			req.buffer.resize(size);

			// other side closed, reading is finished
			// moving the buffer clears it in this
			auto data = wrap(std::move(req.buffer), fmt.c_str());
			for(auto& listener : req.listener) {
				listener(data);
			}

			pending_.erase(it);
		} else if(ret < 0) {
			// EAGAIN: no data available at the moment, continue polling
			if(errno != EAGAIN && errno != EWOULDBLOCK) {
				dlg_warn("read(pipe): {} (error {})", std::strerror(errno),
					errno);

				// unexpected error ocurred, cancel
				for(auto& listener : req.listener) {
					listener(std::monostate {});
				}

				pending_.erase(it);
			}
		} else if(ret < readCount) {
			// we finished reading avilable data
			// try again to see whether we get eof or EAGAIN now
			req.buffer.resize(size + ret); // erase leftover
			continue;
		} else if(ret == readCount) {
			// more data might be avilable, continue
			// let read size grow exponentially for performance
			readCount *= 2;
			continue;
		} else {
			dlg_error("unreachable: ret = {}; readCount = {}",
				ret, readCount);
		}

		break;
	}

	return true;
}

// WaylandDataSource
WaylandDataSource::WaylandDataSource(WaylandAppContext& ac,
	std::unique_ptr<DataSource>&& src, bool dnd) :
		appContext_(ac), source_(std::move(src)), dnd_(dnd) {

	wlDataSource_ = wl_data_device_manager_create_data_source(ac.wlDataManager());
	if(!wlDataSource_) {
		throw std::runtime_error("ny::WaylandDataSource: "
			"failed to create wl_data_source");
	}

	using WDS = WaylandDataSource;
	static constexpr wl_data_source_listener listener = {
		memberCallback<&WDS::target>,
		memberCallback<&WDS::send>,
		memberCallback<&WDS::cancelled>,
		memberCallback<&WDS::dndPerformed>,
		memberCallback<&WDS::dndFinished>,
		memberCallback<&WDS::action>,
	};

	wl_data_source_add_listener(wlDataSource_, &listener, this);

	auto formats = source_->formats();
	for(auto& format : formats) {
		wl_data_source_offer(&wlDataSource(), format.c_str());
	}

	// if this is a dnd source, set the actions and create dnd surface and buffer
	if(dnd_) {
		if(wl_data_source_get_version(wlDataSource_) >=
				WL_DATA_SOURCE_SET_ACTIONS_SINCE_VERSION) {
			auto actions = dndActionsToWayland(source_->supportedActions());
			wl_data_source_set_actions(wlDataSource_, actions);
		}

		auto img = source_->image();
		if(img.data) {
			dragSurface_ = wl_compositor_create_surface(&appContext_.wlCompositor());
			dragBuffer_ = {appContext_, img.size};
			convertFormat(img, waylandToImageFormat(dragBuffer_.format()),
				dragBuffer_.data(), 8u);
		}
	}
}

WaylandDataSource::~WaylandDataSource() {
	if(dragSurface_) {
		wl_surface_destroy(dragSurface_);
	}

	if(wlDataSource_) {
		wl_data_source_destroy(wlDataSource_);
	}
}

void WaylandDataSource::drawSurface() {
	auto img = source_->image();
	wl_surface_attach(dragSurface_, &dragBuffer_.wlBuffer(), 0, 0);
	wl_surface_damage(dragSurface_, 0, 0, img.size[0], img.size[1]);
	wl_surface_commit(dragSurface_);
}

void WaylandDataSource::send(wl_data_source*, const char* mimeType, int32_t fd) {
	// close the fd no matter what happens here
	auto fdGuard = nytl::ScopeGuard([=]{ close(fd); });

	// find the associated DataFormat
	auto formats = source_->formats();
	if(std::find(formats.begin(), formats.end(), mimeType) == formats.end()) {
		dlg_warn("other application requested unsupported mime type: {}",
			mimeType);
		return;
	}

	auto data = source_->data(mimeType);
	if(data.index() == 0) { // invalid
		dlg_warn("failed to retrieve data object via DataSource::data");
		return;
	}

	auto buffer = unwrap(data);

	// TODO: applications might not expect this, might lead to issues
	// at least document this behavior somehow?

	// if the given fd is invalid, no signal should be generated
	// we simply set the sigpipe error handler to ignore and restore it when this
	// function exits
	auto prev = signal(SIGPIPE, SIG_IGN);
	auto sigpipeGuard = nytl::ScopeGuard([=] { signal(SIGPIPE, prev); });

	// NOTE: should we do this nonblocking? and wait for fd to
	// become writable via fdCallback?
	auto ret = write(fd, buffer.data(), buffer.size());
	if(ret < 0) {
		dlg_warn("writing pipe failed: {}", std::strerror(errno));
	}
}

void WaylandDataSource::target(wl_data_source*, const char* mimeType) {
	if(!mimeType) {
		target_.clear();
	} else {
		target_ = mimeType;
	}

	updateCursor();
}

void WaylandDataSource::action(wl_data_source*, uint32_t action) {
	action_ = waylandToDndAction(action);
	source_->action(action_);
	updateCursor();
}

void WaylandDataSource::updateCursor() {
	// change the cursor in response to the action if there is a mouseContext
	if(!appContext_.waylandMouseContext() || !dnd_) {
		return;
	}

	const char* cursorName = "dnd-no-drop";
	if(!target_.empty()) {
		switch(action_) {
			case DndAction::copy:
				cursorName = "dnd-copy";
				break;
			case DndAction::move:
				cursorName = "dnd-move";
				break;
			default: break;
		}
	}

	auto cursor = wl_cursor_theme_get_cursor(appContext_.wlCursorTheme(),
		cursorName);
	auto* img = cursor->images[0];
	auto buffer = wl_cursor_image_get_buffer(img);

	auto hs = nytl::Vec2ui{img->hotspot_x, img->hotspot_y};
	auto size = nytl::Vec2ui{img->width, img->height};

	appContext_.waylandMouseContext()->cursorBuffer(buffer,
		static_cast<nytl::Vec2i>(hs), size);
}

void WaylandDataSource::dndPerformed(wl_data_source*) {
	// we dont care about this information
	// important: do not destroy the source here, only in cancelled/finished
}

void WaylandDataSource::cancelled(wl_data_source*) {
	// destroy self here since this object is no longer needed
	// for dnd sources this means the dnd session ended unsuccesfully
	// for clipboard source this means that it was replaced
	appContext_.destroyDataSource(*this);
}

void WaylandDataSource::dndFinished(wl_data_source*) {
	// destroy self here since this object is no longer needed
	// the dnd session ended succesful and this object will no longer be
	// accessed
	appContext_.destroyDataSource(*this);
}

// WaylandDataDevice
WaylandDataDevice::WaylandDataDevice(WaylandAppContext& ac) : appContext_(&ac) {
	auto wldm = ac.wlDataManager();
	wlDataDevice_ = wl_data_device_manager_get_data_device(wldm, ac.wlSeat());

	using WDD = WaylandDataDevice;
	static constexpr wl_data_device_listener listener = {
		memberCallback<&WDD::offer>,
		memberCallback<&WDD::enter>,
		memberCallback<&WDD::leave>,
		memberCallback<&WDD::motion>,
		memberCallback<&WDD::drop>,
		memberCallback<&WDD::selection>
	};

	wl_data_device_add_listener(wlDataDevice_, &listener, this);
}

WaylandDataDevice::~WaylandDataDevice() {
	if(wlDataDevice_) {
		if(wl_data_device_get_version(wlDataDevice_) >= 2) {
			wl_data_device_release(wlDataDevice_);
		} else {
			wl_data_device_destroy(wlDataDevice_);
		}
	}
}

void WaylandDataDevice::offer(wl_data_device*, wl_data_offer* offer) {
	offers_.push_back(std::make_unique<WaylandDataOffer>(*appContext_, *offer));
}

void WaylandDataDevice::enter(wl_data_device*, uint32_t serial, wl_surface* surface,
		wl_fixed_t x, wl_fixed_t y, wl_data_offer* offer) {
	WaylandEventData eventData(serial);
	nytl::Vec2i pos {wl_fixed_to_int(x), wl_fixed_to_int(y)};

	auto it = findOffer(*offer);
	if(it == offers_.end()) {
		dlg_warn("invalid/unknown wl_data_offer given");
		return;
	}
	dnd_.offer = it->get();

	dnd_.wc = appContext_->windowContext(*surface);
	if(!dnd_.wc) {
		dlg_warn("invalid wl_surface given");
		dnd_ = {};
		return;
	}

	dnd_.serial = serial;

	DndEnterEvent dde;
	dde.eventData = &eventData;
	dde.offer = dnd_.offer;
	dde.position = pos;
	dnd_.wc->listener().dndEnter(dde);

	DndMoveEvent dme;
	dme.eventData = &eventData;
	dme.position = pos;
	dme.offer = dnd_.offer;
	dnd_.wc->listener().dndMove(dme);
}

void WaylandDataDevice::leave(wl_data_device*) {
	// event is always sent, even when data exchange was successful
	// (i.e. drop was sent previously). If we get this
	// event after a drop, dnd_ was already reset and therefore this
	// isn't an error/unexpected situation.
	if(!dnd_.offer || !dnd_.wc) {
		dnd_ = {};
		return;
	}

	auto it = findOffer(*dnd_.offer);
	if(it == offers_.end()) {
		dlg_warn("invalid dnd session offer: {} {} {}");
		dnd_ = {};
		return;
	}
	auto dndOffer = std::move(*it);
	offers_.erase(it);

	DndLeaveEvent dle;
	dle.offer = dndOffer.get();

	auto wc = dnd_.wc;
	dnd_ = {};
	wc->listener().dndLeave(dle);
}

void WaylandDataDevice::motion(wl_data_device*, uint32_t time,
		wl_fixed_t x, wl_fixed_t y) {
	if(!dnd_.offer || !dnd_.wc) {
		dnd_ = {}; // reset
		dlg_warn("no/invalid current dnd session");
		return;
	}

	nytl::unused(time);
	nytl::Vec2i pos{wl_fixed_to_int(x), wl_fixed_to_int(y)};

	DndMoveEvent dme;
	dme.position = pos;
	dme.offer = dnd_.offer;
	dnd_.wc->listener().dndMove(dme);
	dnd_.position = pos;
}

void WaylandDataDevice::drop(wl_data_device*) {
	if(!dnd_.offer || !dnd_.wc) {
		dnd_ = {};
		dlg_warn("no/invalid current dnd session");
		return;
	}

	auto it = findOffer(*dnd_.offer);
	if(it == offers_.end()) {
		dnd_ = {};
		dlg_warn("invalid current dnd offer");
		return;
	}

	auto dndOffer = std::move(*it);
	offers_.erase(it);

	DndDropEvent dde;
	dde.position = dnd_.position;
	dde.offer = std::move(dndOffer);

	// important: reset this *before* we call dndDrop since that
	// might block for some time (recursively dispatching events).
	auto wc = dnd_.wc;
	dnd_ = {};
	wc->listener().dndDrop(dde);
}

void WaylandDataDevice::selection(wl_data_device*, wl_data_offer* offer) {
	// erase the previous clipboard offer
	if(clipboardOffer_) {
		auto it = findOffer(*clipboardOffer_);
		if(it == offers_.end()) {
			dlg_warn("invalid previous clipboard offer");
		} else {
			offers_.erase(it);
		}

		clipboardOffer_ = {};
	}

	if(!offer) {
		return;
	}

	auto it = findOffer(*offer);
	if(it == offers_.end()) {
		dlg_warn("invalid/unknown offer passed as clipboard");
		return;
	}

	clipboardOffer_ = it->get();
}

decltype(WaylandDataDevice::offers_)::iterator
WaylandDataDevice::findOffer(const WaylandDataOffer& offer) {
	return std::find_if(offers_.begin(), offers_.end(), [&](auto& v){
		return v.get() == &offer;
	});
}

decltype(WaylandDataDevice::offers_)::iterator
WaylandDataDevice::findOffer(const wl_data_offer& offer) {
	return std::find_if(offers_.begin(), offers_.end(), [&](auto& v){
		return &v->wlDataOffer() == &offer;
	});
}

} // namespace ny
