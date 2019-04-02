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

//WaylandDataOffer
WaylandDataOffer::WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer)
		: appContext_(&ac), wlDataOffer_(&wlDataOffer) {
	static constexpr wl_data_offer_listener listener {
		memberCallback<&WaylandDataOffer::offer>,
		memberCallback<&WaylandDataOffer::sourceActions>,
		memberCallback<&WaylandDataOffer::action>
	};

	wl_data_offer_add_listener(&wlDataOffer, &listener, this);
}

WaylandDataOffer::WaylandDataOffer(WaylandDataOffer&& other) noexcept :
		appContext_(other.appContext_),
		wlDataOffer_(other.wlDataOffer_),
		formats_(std::move(other.formats_)),
		pending_(std::move(other.pending_)),
		finish_(other.finish_) {
	if(wlDataOffer_) {
		wl_data_offer_set_user_data(wlDataOffer_, this);
	}

	other.appContext_ = {};
	other.wlDataOffer_ = {};
}

WaylandDataOffer& WaylandDataOffer::operator=(WaylandDataOffer&& other) noexcept {
	destroy();

	appContext_ = other.appContext_;
	wlDataOffer_ = other.wlDataOffer_;
	formats_ = std::move(other.formats_);
	pending_ = std::move(other.pending_);
	finish_ = other.finish_;

	if(wlDataOffer_) {
		wl_data_offer_set_user_data(wlDataOffer_, this);
	}

	other.appContext_ = {};
	other.wlDataOffer_ = {};

	return *this;
}

WaylandDataOffer::~WaylandDataOffer() {
	destroy();
}

void WaylandDataOffer::destroy() {
	// signal all pending requests that they have failed
	if(pending_.listener) {
		pending_.listener({});
	}

	if(wlDataOffer_) {
		auto version = wl_data_offer_get_version(wlDataOffer_);
		if(finish_ && version > 3) wl_data_offer_finish(wlDataOffer_);
		wl_data_offer_destroy(wlDataOffer_);
	}
}

bool WaylandDataOffer::formats(FormatsListener listener) {
	listener(formats_);
	return true;
}

bool WaylandDataOffer::data(const char* format, DataListener listener) {
	auto it = std::find(formats_.begin(), formats_.end(), format);
	if(it == formats_.end()) {
		dlg_warn("unsupported format {}", format);
		return false;
	}

	if(pending_.listener) {
		dlg_warn("there is still a data request pending");
		return false;
	}

	pending_.listener = listener;
	pending_.format = format;

	// create the pipe nonblocking, to allow us differentiating
	// eof and no data avilable
	int fds[2];
	auto ret = pipe2(fds, O_CLOEXEC | O_NONBLOCK);
	if(ret < 0) {
		dlg_warn("pipe2 failed: {}", std::strerror(errno));
		return {};
	}

	wl_data_offer_receive(wlDataOffer_, format, fds[1]);

	// we don't capture [this] but instead receive it from
	// the data offers user data to make sure DataOffers can be moved.
	auto callback = [wlOffer = wlDataOffer_, format](int fd, unsigned int) {
		auto& self = *static_cast<WaylandDataOffer*>(
			wl_data_offer_get_user_data(wlOffer));

		auto& buffer = self.pending_.buffer;
		auto readCount = 2048u;
		while(true) {
			auto size = buffer.size();
			buffer.resize(size + readCount);
			auto ret = read(fd, buffer.data() + size, readCount);

			if(ret == 0) {
				// other side closed, reading is finished
				// moving the buffer clears it in this
				self.pending_.listener(wrap(std::move(buffer), format));
				self.pending_.listener = {};
				self.pending_.fdConnection = {};
			} else if(ret < 0) {
				if(errno == EAGAIN || errno == EWOULDBLOCK) {
					// no data avilable at the moment
				} else {
					// unexpected error ocurred, cancel
					self.pending_.listener(std::monostate {});
					self.pending_.listener = {};
					self.pending_.buffer.clear();
					self.pending_.fdConnection = {};
				}
			} else if(ret < readCount) { // fewer available, not finished
				buffer.resize(size + ret); // erase leftover
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
	};

	// also listen to POLLHUP since that might be sent when pipe is closed
	pending_.fdConnection = appContext_->fdCallback(fds[0], POLLIN | POLLHUP,
		callback);
	return true;
}

void WaylandDataOffer::offer(wl_data_offer*, const char* fmt) {
	formats_.push_back(fmt);
}

// TODO: parse actions to determine whether wl_data_offer_finish has to be called? see protocol
void WaylandDataOffer::sourceActions(wl_data_offer*, uint32_t actions) {
	// TODO
}

void WaylandDataOffer::action(wl_data_offer*, uint32_t action) {
	// TODO
	nytl::unused(action);
}

void WaylandDataOffer::removeDataRequest(const std::string& format, DataRequestImpl& request)
{
	auto it = requests_.find(format);
	if(it == requests_.end()) {
		dlg_warn("invalid format");
		return;
	}

	auto it2 = std::find(it->second.requests.begin(), it->second.requests.end(), &request);
	if(it2 == it->second.requests.end()) {
		dlg_warn("invalid request");
		return;
	}

	it->second.requests.erase(it2);
}

// WaylandDataSource
WaylandDataSource::WaylandDataSource(WaylandAppContext& ac, std::unique_ptr<DataSource>&& src,
	bool dnd) : appContext_(ac), source_(std::move(src)), dnd_(dnd)
{
	wlDataSource_ = wl_data_device_manager_create_data_source(ac.wlDataManager());
	if(!wlDataSource_)
		throw std::runtime_error("ny::WaylandDataSource: faield to create wl_data_source");

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
		wl_data_source_offer(&wlDataSource(), format.name.c_str());
		for(auto& name : format.additionalNames)
			wl_data_source_offer(&wlDataSource(), name.c_str());
	}

	// if this is a dnd source, set the actions and create dnd surface and buffer
	if(dnd_) {
		if(wl_data_source_get_version(wlDataSource_) >= 3)
			wl_data_source_set_actions(wlDataSource_, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);

		auto img = source_->image();
		if(img.data) {
			dragSurface_ = wl_compositor_create_surface(&appContext_.wlCompositor());
			dragBuffer_ = {appContext_, img.size};
			convertFormat(img, waylandToImageFormat(dragBuffer_.format()), dragBuffer_.data(), 8u);
		}
	}
}

WaylandDataSource::~WaylandDataSource()
{
	if(dragSurface_) wl_surface_destroy(dragSurface_);
	if(wlDataSource_) wl_data_source_destroy(wlDataSource_);
}

void WaylandDataSource::drawSurface()
{
	auto img = source_->image();
	wl_surface_attach(dragSurface_, &dragBuffer_.wlBuffer(), 0, 0);
	wl_surface_damage(dragSurface_, 0, 0, img.size[0], img.size[1]);
	wl_surface_commit(dragSurface_);
}

void WaylandDataSource::target(wl_data_source*, const char* mimeType)
{
	// TODO: use it to check whether data is accepted (and change cursor)?
	nytl::unused(mimeType);
}

void WaylandDataSource::send(wl_data_source*, const char* mimeType, int32_t fd)
{
	// close the fd no matter what happens here
	auto fdGuard = nytl::ScopeGuard([=]{ close(fd); });

	// find the associated DataFormat
	auto formats = source_->formats();
	DataFormat* dataFormat = nullptr;
	for(auto& format : formats) if(match(format, mimeType)) dataFormat = &format;

	if(!dataFormat) {
		dlg_warn("invalid/unsupported mimeType: {}", mimeType);
		return;
	}

	auto data = source_->data(*dataFormat);
	if(!data.has_value()) {
		dlg_warn("failed to retrieve data object via DataSource::data");
		return;
	}

	auto buffer = unwrap(data, *dataFormat);

	// if the given fd is invalid, no signal should be generated
	// we simply set the sigpipe error handler to ignore and restore it when this
	// function exits
	auto prev = signal(SIGPIPE, SIG_IGN);
	auto sigpipeGuard = nytl::ScopeGuard([=] { signal(SIGPIPE, prev); });

	auto ret = write(fd, buffer.data(), buffer.size());
	if(ret < 0) dlg_warn("write failed: {}", std::strerror(errno));
}

void WaylandDataSource::action(wl_data_source*, uint32_t action)
{
	// change the cursor in response to the action if there is a mouseContext
	if(!appContext_.waylandMouseContext() || !dnd_) return;

	const char* cursorName = nullptr;
	switch(action) {
		case WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY: cursorName = "dnd-copy"; break;
		case WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE: cursorName = "dnd-move"; break;
		default: cursorName = "dnd-none";
	}

	auto cursor = wl_cursor_theme_get_cursor(appContext_.wlCursorTheme(), cursorName);
	auto* img = cursor->images[0];
	auto buffer = wl_cursor_image_get_buffer(img);

	auto hs = nytl::Vec2ui{img->hotspot_x, img->hotspot_y};
	auto size = nytl::Vec2ui{img->width, img->height};

	appContext_.waylandMouseContext()->cursorBuffer(buffer, static_cast<nytl::Vec2i>(hs), size);
}

void WaylandDataSource::dndPerformed(wl_data_source*)
{
	// we dont care about this information
	// important: do not destroy the source here, only in cancelled/finished
}

void WaylandDataSource::cancelled(wl_data_source*)
{
	// destroy self here since this object is no longer needed
	// for dnd sources this means the dnd session ended unsuccesfully
	// for clipboard source this means that it was replaced
	appContext_.destroyDataSource(*this);
}

void WaylandDataSource::dndFinished(wl_data_source*)
{
	// destroy self here since this object is no longer needed
	// the dnd session ended succesful and this object will no longer be
	// accessed
	appContext_.destroyDataSource(*this);
}

//WaylandDataDevice
WaylandDataDevice::WaylandDataDevice(WaylandAppContext& ac) : appContext_(&ac)
{
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

WaylandDataDevice::~WaylandDataDevice()
{
	if(wlDataDevice_) {
		if(wl_data_device_get_version(wlDataDevice_) >= 2) wl_data_device_release(wlDataDevice_);
		else wl_data_device_destroy(wlDataDevice_);
	}
}

void WaylandDataDevice::offer(wl_data_device*, wl_data_offer* offer)
{
	offers_.push_back(std::make_unique<WaylandDataOffer>(*appContext_, *offer));
}

void WaylandDataDevice::enter(wl_data_device*, uint32_t serial, wl_surface* surface,
	wl_fixed_t x, wl_fixed_t y, wl_data_offer* offer)
{
	WaylandEventData eventData(serial);
	nytl::Vec2i pos{wl_fixed_to_int(x), wl_fixed_to_int(y)};

	// find the associated dataOffer and cache it as dndOffer_
	for(auto& o : offers_) {
		if(&o->wlDataOffer() == offer) {
			dndOffer_ = o.get();
			break;
		}
	}

	if(!dndOffer_) {
		dlg_info("invalid wl_data_offer given");
		return;
	}

	dndWC_ = appContext_->windowContext(*surface);
	if(!dndWC_) {
		dlg_info("invalid wl_surface given");
		dndOffer_ = {};
		return;
	}

	dndSerial_ = serial;

	DndEnterEvent dde;
	dde.eventData = &eventData;
	dde.offer = dndOffer_;
	dde.position = pos;
	dndWC_->listener().dndEnter(dde);

	// TODO: already handle dndMove return format
	DndMoveEvent dme;
	dme.eventData = &eventData;
	dme.position = pos;
	dme.offer = dndOffer_;
	dndWC_->listener().dndMove(dme);
}

void WaylandDataDevice::leave(wl_data_device*)
{
	if(dndOffer_) {
		auto it = std::find_if(offers_.begin(), offers_.end(),
			[=](auto& v) { return v.get() == dndOffer_; });

		if(it != offers_.end()) {
			DndLeaveEvent dle;
			dle.offer = it->get();
			if(dndWC_) dndWC_->listener().dndLeave(dle);
			offers_.erase(it);
		}
	}

	dndOffer_ = {};
	dndSerial_ = {};
	dndWC_ = {};
}

void WaylandDataDevice::motion(wl_data_device*, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	// debug("motion");
	nytl::unused(time); //time param needed for CompFunc since it would be stored in x otherwise
	nytl::Vec2i pos{wl_fixed_to_int(x), wl_fixed_to_int(y)};

	DndMoveEvent dme;
	dme.position = pos;
	dme.offer = dndOffer_;
	auto fmt = dndWC_->listener().dndMove(dme);

	if(fmt == DataFormat::none) {
		wl_data_offer_accept(&dndOffer_->wlDataOffer(), dndSerial_, nullptr);
		if(wl_data_offer_get_version(&dndOffer_->wlDataOffer()) >= 3) {
			wl_data_offer_set_actions(&dndOffer_->wlDataOffer(),
				WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE,
				WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE);
		}
		return;
	}

	wl_data_offer_accept(&dndOffer_->wlDataOffer(), dndSerial_, fmt.name.c_str());
	// wl_data_offer_set_actions(&dndOffer_->wlDataOffer(), WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE,
		// WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);
	wl_data_offer_set_actions(&dndOffer_->wlDataOffer(), WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY,
		WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
}

void WaylandDataDevice::drop(wl_data_device*)
{
	// debug("ny::WaylandDataDevice::drop");

	if(!dndOffer_ || !dndWC_) {
		dlg_info("invalid current dnd session");
		return;
	}

	std::unique_ptr<WaylandDataOffer> ownedDndOffer;
	for(auto it = offers_.begin(); it != offers_.end(); ++it) {
		if(it->get() == dndOffer_) {
			ownedDndOffer = std::move(*it);
			offers_.erase(it);
			break;
		}
	}

	if(ownedDndOffer) {
		ownedDndOffer->finish(true);
		if(dndWC_) {
			DndDropEvent dde;
			dde.position = {}; // TODO
			dde.offer = std::move(ownedDndOffer);
			dndWC_->listener().dndDrop(dde);
		} else {
			dlg_warn("no current dnd WindowContext");
		}
	} else {
		dlg_warn("invalid current cached dnd offer");
	}

	dndOffer_ = {};
	dndSerial_ = {};
	dndWC_ = {};
}

void WaylandDataDevice::selection(wl_data_device*, wl_data_offer* offer)
{
	// erase the previous clipboard offer
	if(clipboardOffer_) {
		offers_.erase(std::remove_if(offers_.begin(), offers_.end(),
			[=](auto& v) { return v.get() == clipboardOffer_; }), offers_.end());
		clipboardOffer_ = nullptr;
	}

	if(!offer) return;
	for(auto& o : offers_) {
		if(&o->wlDataOffer() == offer) {
			clipboardOffer_ = o.get();
			return;
		}
	}

	dlg_warn("unkown offer argument");
}

} // namespace ny
