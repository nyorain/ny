// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/data.hpp>
#include <ny/wayland/util.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/log.hpp>

#include <nytl/stringParam.hpp>
#include <nytl/scope.hpp>

#include <wayland-client-protocol.h>
#include <wayland-cursor.h>

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>

#include <algorithm>

namespace ny
{

namespace
{

///Small DefaultAsyncRequest addition that allow more to store a nytl::ConnectionGuard
///in the Request objects.
class WaylandDataOfferDataRequest : public DefaultAsyncRequest<std::any>
{
public:
	using DefaultAsyncRequest::DefaultAsyncRequest;
	nytl::ConnectionGuard connection;
};

}

//WaylandDataOffer
WaylandDataOffer::WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer, bool dnd)
	: appContext_(&ac), wlDataOffer_(&wlDataOffer), dnd_(dnd)
{
	static constexpr wl_data_offer_listener listener =
	{
		memberCallback<decltype(&WaylandDataOffer::offer),
			&WaylandDataOffer::offer, void(wl_data_offer*, const char*)>,

		memberCallback<decltype(&WaylandDataOffer::sourceActions),
			&WaylandDataOffer::sourceActions, void(wl_data_offer*, unsigned int)>,

		memberCallback<decltype(&WaylandDataOffer::action),
			&WaylandDataOffer::action, void(wl_data_offer*, unsigned int)>,
	};

	wl_data_offer_add_listener(&wlDataOffer, &listener, this);
}

WaylandDataOffer::WaylandDataOffer(WaylandDataOffer&& other) noexcept :
	appContext_(other.appContext_),
	wlDataOffer_(other.wlDataOffer_),
	formats_(std::move(other.formats_)),
	dnd_(other.dnd_),
	requests_(std::move(other.requests_))
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
	dnd_ = other.dnd_;

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
	for(auto& r : requests_) r.second.callback({});

	if(wlDataOffer_)
	{
		if(dnd_) wl_data_offer_finish(wlDataOffer_);
		wl_data_offer_destroy(wlDataOffer_);
	}
}

WaylandDataOffer::FormatsRequest WaylandDataOffer::formats() const
{
	//Since we don't have to query the supported formats but already have them
	//stored, we can return a synchronous (i.e. already set) request object.
	std::vector<DataFormat> formats;
	formats.reserve(formats_.size());
	for(auto& supported : formats_) formats.push_back(supported.first);
	return std::make_unique<DefaultAsyncRequest<std::vector<DataFormat>>>(formats);
}

WaylandDataOffer::DataRequest WaylandDataOffer::data(const DataFormat& format)
{
	//find the associated wayland format string
	std::pair<DataFormat, std::string> reqfmt {};
	for(auto& supported : formats_) if(format == supported.first) reqfmt = supported;
	if(reqfmt.second.empty())
	{
		warning("ny::WaylandDataOffer::data: unsupported format ", format.name);
		return {};
	}

	//we check if there is already a pending request for the given format.
	//if so, we skip all request and appContext fd callback registering
	auto& conn = requests_[reqfmt.second].fdConnection;
	if(!conn.connected())
	{
		int fds[2];
		auto ret = pipe2(fds, O_CLOEXEC);
		if(ret < 0)
		{
			warning("ny::WaylandDataOffer::data: pipe2 failed: ", std::strerror(errno));
			return {};
		}

		wl_data_offer_receive(wlDataOffer_, reqfmt.second.c_str(), fds[1]);

		auto callback = [wlOffer = wlDataOffer_,
			ac = appContext_, format, reqfmt] (int fd)
		{
			//close the fd no matter if we actually fail here
			auto fdGuard = nytl::makeScopeGuard([=]{ close(fd); });

			constexpr auto readCount = 1000;
			auto self = static_cast<WaylandDataOffer*>(wl_data_offer_get_user_data(wlOffer));
			std::vector<uint8_t> buffer;

			auto ret = readCount;
			while(ret == readCount)
			{
				buffer.resize(buffer.size() + readCount);
				ret = read(fd, buffer.data(), buffer.size());
			}

			buffer.resize(buffer.size() - (readCount - ret)); //remove the unneeded bytes
			//is eof really assured here?
			//the data source side might write multiple data segments and not be
			//finished here...

			auto any = wrap(buffer, reqfmt.first);

			self->requests_[reqfmt.second].callback(any);
			self->requests_.erase(self->requests_.find(reqfmt.second));
		};

		conn = appContext_->fdCallback(fds[0], POLLIN, callback);
	}

	//create an asynchronous request object that keeps a connection to the
	//callback that will be triggered from within the fd callback, i.e. when
	//the data is received.
	auto ret = std::make_unique<WaylandDataOfferDataRequest>(appContext());
	auto completeFunc = [ptr = ret.get()](const std::any& any) { ptr->complete(any); };
	// ret->connection = requests_[reqfmt.second].callback.add(completeFunc);
	requests_[reqfmt.second].callback.add(completeFunc);
	return ret;
}

void WaylandDataOffer::offer(const char* format)
{
	if(match(DataFormat::raw, format)) formats_.push_back({DataFormat::raw, format});
	if(match(DataFormat::text, format)) formats_.push_back({DataFormat::text, format});
	if(match(DataFormat::uriList, format)) formats_.push_back({DataFormat::uriList, format});
	if(match(DataFormat::imageData, format)) formats_.push_back({DataFormat::imageData, format});

	formats_.push_back({{format, {}}, format,});
	// wl_data_offer_accept(wlDataOffer_, 0, mimeType);
}

void WaylandDataOffer::sourceActions(unsigned int actions)
{
	nytl::unused(actions);
}

void WaylandDataOffer::action(unsigned int action)
{
	nytl::unused(action);
	// debug("action: ", action);
}

//WaylandDataSource
WaylandDataSource::WaylandDataSource(WaylandAppContext& ac, std::unique_ptr<DataSource> src,
	bool dnd) : appContext_(ac), source_(std::move(src)), dnd_(dnd)
{
	wlDataSource_ = wl_data_device_manager_create_data_source(ac.wlDataManager());

	using WDS = WaylandDataSource;
	static constexpr wl_data_source_listener listener =
	{
		memberCallback<decltype(&WDS::target), &WDS::target, void(wl_data_source*, const char*)>,
		memberCallback<decltype(&WDS::send), &WDS::send, void(wl_data_source*, const char*, int)>,
		memberCallback<decltype(&WDS::cancelled), &WDS::cancelled, void(wl_data_source*)>,
		memberCallback<decltype(&WDS::dndPerformed), &WDS::dndPerformed, void(wl_data_source*)>,
		memberCallback<decltype(&WDS::dndFinished), &WDS::dndFinished, void(wl_data_source*)>,
		memberCallback<decltype(&WDS::action), &WDS::action, void(wl_data_source*, unsigned int)>,
	};

	wl_data_source_add_listener(wlDataSource_, &listener, this);

	auto formats = source_->formats();
	for(auto& format : formats)
	{
		wl_data_source_offer(&wlDataSource(), format.name.c_str());
		for(auto& name : format.additionalNames)
			wl_data_source_offer(&wlDataSource(), name.c_str());
	}

	if(dnd_) wl_data_source_set_actions(wlDataSource_, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
}

WaylandDataSource::~WaylandDataSource()
{
	if(wlDataSource_) wl_data_source_destroy(wlDataSource_);
}

void WaylandDataSource::target(const char* mimeType)
{
	//we dont care about this information
	nytl::unused(mimeType);
	// debug("target ", mimeType);
}
void WaylandDataSource::send(const char* mimeType, int fd)
{
	//close the fd no matter what
	auto fdGuard = nytl::makeScopeGuard([=]{ close(fd); });

	//find the associated DataFormat
	auto formats = source_->formats();
	DataFormat* dataFormat = nullptr;
	for(auto& format : formats) if(match(format, mimeType)) dataFormat = &format;

	if(!dataFormat)
	{
		log("ny::WaylandDataSource::send: invalid/unsupported mimeType: ", mimeType);
		return;
	}

	auto data = source_->data(*dataFormat);
	if(!data.has_value())
	{
		warning("ny::WaylandDataSource::send: failed to retrieve data object via DataSource::data");
		return;
	}

	auto buffer = unwrap(data, *dataFormat);
	write(fd, buffer.data(), buffer.size());
}

void WaylandDataSource::action(unsigned int action)
{
	// debug("action ", action);

	const char* cursorName = nullptr;
	switch(action)
	{
		case WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY: cursorName = "dnd-copy"; break;
		case WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE: cursorName = "dnd-move"; break;
		default: cursorName = "dnd-none";
	}

	auto cursor = wl_cursor_theme_get_cursor(appContext_.wlCursorTheme(), cursorName);
	auto* img = cursor->images[0];
	auto buffer = wl_cursor_image_get_buffer(img);

	auto hotspot = nytl::Vec2i(img->hotspot_x, img->hotspot_y);
	auto size = nytl::Vec2i(img->width, img->height);

	if(appContext_.waylandMouseContext())
		appContext_.waylandMouseContext()->cursorBuffer(buffer, hotspot, size);
}

void WaylandDataSource::dndPerformed()
{
	//we dont care about this information
	//important: do not destroy the source here, only in cancelled/finished
	// debug("performed");
}

void WaylandDataSource::cancelled()
{
	//destroy self here
	// debug("cancelled");
	delete this;
}

void WaylandDataSource::dndFinished()
{
	//destroy self here
	// debug("finished");
	delete this;
}

//WaylandDataDevice
WaylandDataDevice::WaylandDataDevice(WaylandAppContext& ac) : appContext_(&ac)
{
	auto wldm = ac.wlDataManager();
	wlDataDevice_ = wl_data_device_manager_get_data_device(wldm, ac.wlSeat());

	using WDD = WaylandDataDevice;
	static constexpr wl_data_device_listener listener =
	{
		memberCallback<decltype(&WDD::offer), &WDD::offer, void(wl_data_device*, wl_data_offer*)>,
		memberCallback<decltype(&WDD::enter), &WDD::enter,
			void(wl_data_device*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t, wl_data_offer*)>,
		memberCallback<decltype(&WDD::leave), &WDD::leave, void(wl_data_device*)>,
		memberCallback<decltype(&WDD::motion), &WDD::motion,
			void(wl_data_device*, uint32_t, wl_fixed_t, wl_fixed_t)>,
		memberCallback<decltype(&WDD::drop), &WDD::drop, void(wl_data_device*)>,
		memberCallback<decltype(&WDD::selection),
			&WDD::selection, void(wl_data_device*, wl_data_offer*)>,
	};

	wl_data_device_add_listener(wlDataDevice_, &listener, this);
}

WaylandDataDevice::~WaylandDataDevice()
{
	if(wlDataDevice_) wl_data_device_destroy(wlDataDevice_);
}

void WaylandDataDevice::offer(wl_data_offer* offer)
{
	// debug("offer");
	offers_.push_back(std::make_unique<WaylandDataOffer>(*appContext_, *offer));
}

void WaylandDataDevice::enter(unsigned int serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y,
	wl_data_offer* offer)
{
	nytl::unused(serial,x, y);
	dndWC_ = appContext_->windowContext(*surface);

	for(auto& o : offers_)
	{
		if(&o->wlDataOffer() == offer)
		{
			dndOffer_ = o.get();
			dndOffer_->dnd(true);
			break;
		}
	}
}

void WaylandDataDevice::leave()
{
	// debug("leave");
	if(dndOffer_)
	{
		offers_.erase(std::remove_if(offers_.begin(), offers_.end(),
			[=](auto& v) { return v.get() == dndOffer_; }), offers_.end());

		dndOffer_ = nullptr;
	}
}

void WaylandDataDevice::motion(unsigned int time, wl_fixed_t x, wl_fixed_t y)
{
	// debug("motion");
	nytl::unused(time, x, y);

	wl_data_offer_set_actions(&dndOffer_->wlDataOffer(), WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE,
		WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);
}

void WaylandDataDevice::drop()
{
	// debug("drop");
	if(!dndOffer_)
	{
		log("ny::WaylandDataDevice::drop: invalid current dnd session.");
		return;
	}

	std::unique_ptr<WaylandDataOffer> ownedDndOffer;
	for(auto it = offers_.begin(); it != offers_.end(); ++it)
	{
		if(it->get() == dndOffer_)
		{
			ownedDndOffer = std::move(*it);
			offers_.erase(it);
			break;
		}
	}

	if(dndWC_ && dndWC_->eventHandler())
	{
		// debug("drop drop");
		DataOfferEvent event(dndWC_->eventHandler());
		event.offer = std::move(ownedDndOffer);
		appContext_->dispatch(std::move(event));
	}

	dndOffer_ = nullptr;
}

void WaylandDataDevice::selection(wl_data_offer* offer)
{
	// debug("selection");
	if(clipboardOffer_)
	{
		offers_.erase(std::remove_if(offers_.begin(), offers_.end(),
			[=](auto& v) { return v.get() == clipboardOffer_; }), offers_.end());
	}

	if(!offer) return;
	for(auto& o : offers_)
	{
		if(&o->wlDataOffer() == offer)
		{
			clipboardOffer_ = o.get();
			clipboardOffer_->dnd(false);
			return;
		}
	}

	warning("ny::WaylandDataDevice::selection: unkown offer argument");
}

}
