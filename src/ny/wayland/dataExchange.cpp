// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/dataExchange.hpp>
#include <ny/wayland/util.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/log.hpp>
#include <ny/asyncRequest.hpp>

#include <nytl/stringParam.hpp>
#include <nytl/scope.hpp>

#include <wayland-client-protocol.h>
#include <wayland-cursor.h>

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <signal.h>

#include <algorithm>

namespace ny {

// /Represents a pending wayland to another request for a specific format.
// /Will be associated with a format using a std::map.
class WaylandDataOffer::PendingRequest {
public:
	std::vector<WaylandDataOffer::DataRequestImpl*> requests;
	nytl::UniqueConnection fdConnection;
};

// /Small DefaultAsyncRequest addition that allows to unregister itself on desctruction.
class WaylandDataOffer::DataRequestImpl : public DefaultAsyncRequest<std::any> {
public:
	using DefaultAsyncRequest::DefaultAsyncRequest;

	WaylandDataOffer* dataOffer_;
	std::string format_;

	~DataRequestImpl()
	{
		if(dataOffer_) dataOffer_->removeDataRequest(format_, *this);
	}

	void complete(const std::any& any)
	{
		DefaultAsyncRequest::complete(std::any(any));
		dataOffer_ = nullptr;
	}
};

//WaylandDataOffer
WaylandDataOffer::WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer)
	: appContext_(&ac), wlDataOffer_(&wlDataOffer)
{
	static constexpr wl_data_offer_listener listener {
		memberCallback<decltype(&WaylandDataOffer::offer), &WaylandDataOffer::offer>,
		memberCallback<decltype(&WaylandDataOffer::sourceActions),
			&WaylandDataOffer::sourceActions>,
		memberCallback<decltype(&WaylandDataOffer::action), &WaylandDataOffer::action>
	};

	wl_data_offer_add_listener(&wlDataOffer, &listener, this);
}

WaylandDataOffer::WaylandDataOffer(WaylandDataOffer&& other) noexcept :
	appContext_(other.appContext_),
	wlDataOffer_(other.wlDataOffer_),
	formats_(std::move(other.formats_)),
	requests_(std::move(other.requests_)),
	finish_(other.finish_)
{
	if(wlDataOffer_) wl_data_offer_set_user_data(wlDataOffer_, this);

	other.appContext_ = {};
	other.wlDataOffer_ = {};
}

WaylandDataOffer& WaylandDataOffer::operator=(WaylandDataOffer&& other) noexcept
{
	destroy();

	appContext_ = other.appContext_;
	wlDataOffer_ = other.wlDataOffer_;
	formats_ = std::move(other.formats_);
	requests_ = std::move(other.requests_);
	finish_ = other.finish_;

	if(wlDataOffer_) wl_data_offer_set_user_data(wlDataOffer_, this);

	other.appContext_ = {};
	other.wlDataOffer_ = {};

	return *this;
}

WaylandDataOffer::~WaylandDataOffer()
{
	destroy();
}

void WaylandDataOffer::destroy()
{
	// signal all pending requests that they have failed
	for(auto& r : requests_) for(auto& req : r.second.requests) req->complete({});

	if(wlDataOffer_) {
		auto version = wl_data_offer_get_version(wlDataOffer_);
		if(finish_ && version > 3) wl_data_offer_finish(wlDataOffer_);
		wl_data_offer_destroy(wlDataOffer_);
	}
}

WaylandDataOffer::FormatsRequest WaylandDataOffer::formats()
{
	// Since we don't have to query the supported formats but already have them
	// stored, we can return a synchronous (i.e. already set) request object.
	std::vector<DataFormat> formats;
	formats.reserve(formats_.size());
	for(auto& supported : formats_) formats.push_back(supported.first);
	return std::make_unique<DefaultAsyncRequest<std::vector<DataFormat>>>(formats);
}

WaylandDataOffer::DataRequest WaylandDataOffer::data(const DataFormat& format)
{
	dlg::SourceGuard("::WaylandDataOffer::data"_src);

	// find the associated wayland format string
	std::pair<DataFormat, std::string> reqfmt {};
	for(auto& supported : formats_) if(format == supported.first) reqfmt = supported;
	if(reqfmt.second.empty()) {
		ny_warn("unsupported format {}", format.name);
		return {};
	}

	// we check if there is already a pending request for the given format.
	// if so, we skip all request and appContext fd callback registering
	auto& conn = requests_[reqfmt.second].fdConnection;
	if(!conn.connected()) {
		int fds[2];
		auto ret = pipe2(fds, O_CLOEXEC);
		if(ret < 0) {
			ny_warn("pipe2 failed: {}", std::strerror(errno));
			return {};
		}

		wl_data_offer_receive(wlDataOffer_, reqfmt.second.c_str(), fds[1]);

		auto callback =
			[wlOffer = wlDataOffer_, ac = appContext_, format, reqfmt](int fd, unsigned int) {

			auto fdGuard = nytl::makeScopeGuard([=]{ close(fd); });

			constexpr auto readCount = 1000;
			auto self = static_cast<WaylandDataOffer*>(wl_data_offer_get_user_data(wlOffer));
			std::vector<uint8_t> buffer;

			auto ret = readCount;
			while(ret == readCount) {
				buffer.resize(buffer.size() + readCount);
				ret = read(fd, buffer.data(), buffer.size());
			}

			buffer.resize(buffer.size() - (readCount - ret)); // remove the unneeded bytes
			// TODO: is eof really assured here?
			// the data source side might write multiple data segments and not be
			// finished here...

			auto any = wrap(buffer, reqfmt.first);

			// complete all pending requests and remove the request entry
			for(auto& req : self->requests_[reqfmt.second].requests) req->complete(any);
			self->requests_.erase(self->requests_.find(reqfmt.second));
		};

		conn = appContext_->fdCallback(fds[0], POLLIN, callback);
	}

	// create an asynchronous request object that unregisters itself on destruction so
	// we won't call complete on invalid objects. The application has ownership over the
	// AsyncRequest
	auto ret = std::make_unique<DataRequestImpl>(appContext());
	ret->format_ = reqfmt.second;
	ret->dataOffer_ = this;
	requests_[reqfmt.second].requests.push_back(ret.get());
	return ret;
}

void WaylandDataOffer::offer(wl_data_offer*, const char* fmt)
{
	if(match(DataFormat::raw, fmt)) formats_.push_back({DataFormat::raw, fmt});
	else if(match(DataFormat::text, fmt)) formats_.push_back({DataFormat::text, fmt});
	else if(match(DataFormat::uriList, fmt)) formats_.push_back({DataFormat::uriList, fmt});
	else if(match(DataFormat::image, fmt)) formats_.push_back({DataFormat::image, fmt});
	else formats_.push_back({{fmt, {}}, fmt,});
}

// TODO: parse actions to determine whether wl_data_offer_finish has to be called? see protocol
void WaylandDataOffer::sourceActions(wl_data_offer*, uint32_t actions)
{
	nytl::unused(actions);
}

void WaylandDataOffer::action(wl_data_offer*, uint32_t action)
{
	nytl::unused(action);
}

void WaylandDataOffer::removeDataRequest(const std::string& format, DataRequestImpl& request)
{
	auto it = requests_.find(format);
	if(it == requests_.end()) {
		ny_warn("::WaylandDataOffer::removeDataRequest"_src, "invalid format");
		return;
	}

	auto it2 = std::find(it->second.requests.begin(), it->second.requests.end(), &request);
	if(it2 == it->second.requests.end()) {
		ny_warn("::WaylandDataOffer::removeDataRequest"_src ,"invalid request");
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
		memberCallback<decltype(&WDS::target), &WDS::target, void(wl_data_source*, const char*)>,
		memberCallback<decltype(&WDS::send), &WDS::send, void(wl_data_source*, const char*, int)>,
		memberCallback<decltype(&WDS::cancelled), &WDS::cancelled, void(wl_data_source*)>,
		memberCallback<decltype(&WDS::dndPerformed), &WDS::dndPerformed, void(wl_data_source*)>,
		memberCallback<decltype(&WDS::dndFinished), &WDS::dndFinished, void(wl_data_source*)>,
		memberCallback<decltype(&WDS::action), &WDS::action, void(wl_data_source*, unsigned int)>,
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
	dlg::SourceGuard sourceGuard("::WaylandDataSource::send"_src);

	// close the fd no matter what happens here
	auto fdGuard = nytl::makeScopeGuard([=]{ close(fd); });

	// find the associated DataFormat
	auto formats = source_->formats();
	DataFormat* dataFormat = nullptr;
	for(auto& format : formats) if(match(format, mimeType)) dataFormat = &format;

	if(!dataFormat) {
		ny_warn("invalid/unsupported mimeType: {}", mimeType);
		return;
	}

	auto data = source_->data(*dataFormat);
	if(!data.has_value()) {
		ny_warn("failed to retrieve data object via DataSource::data");
		return;
	}

	auto buffer = unwrap(data, *dataFormat);

	// if the given fd is invalid, no signal should be generated
	// we simply set the sigpipe error handler to ignore and restore it when this
	// function exits
	auto prev = signal(SIGPIPE, SIG_IGN);
	auto sigpipeGuard = nytl::makeScopeGuard([=] { signal(SIGPIPE, prev); });

	auto ret = write(fd, buffer.data(), buffer.size());
	if(ret < 0) ny_warn("write failed: {}", std::strerror(errno));
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
		memberCallback<decltype(&WDD::offer), &WDD::offer>,
		memberCallback<decltype(&WDD::enter), &WDD::enter>,
		memberCallback<decltype(&WDD::leave), &WDD::leave>,
		memberCallback<decltype(&WDD::motion), &WDD::motion>,
		memberCallback<decltype(&WDD::drop), &WDD::drop>,
		memberCallback<decltype(&WDD::selection), &WDD::selection>
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
		ny_info("::WaylandDataDevice::enter"_src, "invalid wl_data_offer given");
		return;
	}

	dndWC_ = appContext_->windowContext(*surface);
	if(!dndWC_) {
		ny_info("::WaylandDataDevice::enter:"_src, "invalid wl_surface given");
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
	wl_data_offer_set_actions(&dndOffer_->wlDataOffer(), WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY,
		WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
}

void WaylandDataDevice::drop(wl_data_device*)
{
	// debug("ny::WaylandDataDevice::drop");

	if(!dndOffer_ || !dndWC_) {
		ny_info("::WaylandDataDevice::drop"_src, "invalid current dnd session");
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
			ny_warn("::WaylandDataDevice::drop"_src, "no current dnd WindowContext");
		}
	} else {
		ny_warn("::WaylandDataDevice::drop"_src, "invalid current cached dnd offer");
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

	ny_warn("::WaylandDataDevice::selection"_src, "unkown offer argument");
}

} // namespace ny
