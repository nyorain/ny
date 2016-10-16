#include <ny/backend/wayland/data.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/input.hpp>
#include <ny/base/log.hpp>

#include <nytl/stringParam.hpp>

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
	bool comp(const char* mimeType, const char* received)
	{
		return strncmp(mimeType, received, strlen(mimeType)) == 0;
	}

	//TODO: could be part of the public interface
	//TODO: still many missing
	//NOTE: this is total bullshit and does not really make sense for correct impl...
	unsigned int mimeTypeToFormat(nytl::StringParam param)
	{
		if(comp("text/plain", param)) return dataType::text;

		if(param == "image/png") return dataType::png;
		if(param == "image/bmp") return dataType::bmp;
		if(param == "image/jpeg") return dataType::jpeg;
		if(param == "image/gif") return dataType::gif;

		if(param == "audio/mpeg") return dataType::mp3;
		if(param == "audio/mpeg3") return dataType::mp3;

		return dataType::none;
	}

	const char* formatToMimeType(unsigned int fmt)
	{
		switch(fmt)
		{
			case dataType::text: return "text/plain;charset=utf-8";
			case dataType::png: return "image/png";
			case dataType::bmp: return "image/bmp";
			case dataType::jpeg: return "image/jpeg";
			case dataType::gif: return "image/gif";
			case dataType::mp3: return "image/mpeg";
		}

		return nullptr;
	}

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
	dataTypes_(std::move(other.dataTypes_)), 
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
	dataTypes_ = std::move(other.dataTypes_);
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
	for(auto& r : requests_)
	{
		r.second.callback({}, *this, r.first);
		r.second.connection.destroy();
	}

	if(wlDataOffer_)
	{
		if(dnd_) wl_data_offer_finish(wlDataOffer_);
		wl_data_offer_destroy(wlDataOffer_);
	}
}

nytl::CbConn WaylandDataOffer::data(unsigned int fmt, const DataOffer::DataFunction& func)
{
	auto mimeType = formatToMimeType(fmt);
	if(!mimeType)
	{
		warning("WaylandDataOffer::data: unsupported format.");
		func({}, *this, fmt);
		return {};
	}

	auto& conn = requests_[fmt].connection;

	//we check if there is already a pending request for the given format.
	//if so, we skip all request and appContext fd callback registering
	if(!conn.connected())
	{
		int fds[2];
		auto ret = pipe2(fds, O_CLOEXEC);
		if(ret < 0)
		{
			warning("WaylandDataOffer::data: pipe2 failed.");
			func({}, *this, fmt);
			return {};
		}

		wl_data_offer_receive(wlDataOffer_, "text/plain;charset=utf-8", fds[1]);
		debug("receive callled...");

		auto callback = [wlOffer = wlDataOffer_, ac = appContext_, fmt](int fd, unsigned int re)
		{
			debug("callback, ", re);
			constexpr auto readCount = 1000;
			auto self = static_cast<WaylandDataOffer*>(wl_data_offer_get_user_data(wlOffer));
			auto& buffer = self->requests_[fmt].buffer;

			auto ret = 0;
			do
			{
				buffer.resize(buffer.size() + readCount);
				ret = read(fd, buffer.data(), buffer.size());
			}
			while(ret == readCount);

			//is eof really assured here? 
			//the data source side might write multiple data segments and not be 
			//finished here...
			std::any any;
			if(fmt == dataType::text) any = std::string(buffer.begin(), buffer.end());
			else any = buffer;

			self->requests_[fmt].callback(any, *self, fmt);
			self->requests_[fmt].callback.clear();
			self->requests_[fmt].connection.destroy();
			self->requests_[fmt].buffer.clear();
			close(fd);
		};
		
		conn = appContext_->fdCallback(fds[0], POLLIN | POLLHUP, callback);
	}

	return requests_[fmt].callback.add(func);
}

void WaylandDataOffer::offer(const char* mimeType)
{
	auto fmt = mimeTypeToFormat(mimeType);
	// log("ny::WaylandDataOffer::offer: ", mimeType);
	// log("ny::WaylandDataOffer::offer format: ", fmt);

	if(fmt != dataType::none)
	{
		dataTypes_.add(fmt);
		wl_data_offer_accept(wlDataOffer_, 0, mimeType);
	}
}

void WaylandDataOffer::sourceActions(unsigned int actions)
{
	nytl::unused(actions);
}

void WaylandDataOffer::action(unsigned int action)
{
	nytl::unused(action);
	debug("action: ", action);
}

//WaylandDataSource
WaylandDataSource::WaylandDataSource(WaylandAppContext& ac, std::unique_ptr<DataSource> src, 
	bool dnd) : appContext_(ac), source_(std::move(src)), dnd_(dnd)
{
	wlDataSource_ = wl_data_device_manager_create_data_source(ac.wlDataManager());

	static constexpr wl_data_source_listener listener =
	{
		memberCallback<decltype(&WaylandDataSource::target), 
			&WaylandDataSource::target, void(wl_data_source*, const char*)>,

		memberCallback<decltype(&WaylandDataSource::send), 
			&WaylandDataSource::send, void(wl_data_source*, const char*, int)>,

		memberCallback<decltype(&WaylandDataSource::cancelled), 
			&WaylandDataSource::cancelled, void(wl_data_source*)>,

		memberCallback<decltype(&WaylandDataSource::dndPerformed), 
			&WaylandDataSource::dndPerformed, void(wl_data_source*)>,

		memberCallback<decltype(&WaylandDataSource::dndFinished), 
			&WaylandDataSource::dndFinished, void(wl_data_source*)>,

		memberCallback<decltype(&WaylandDataSource::action), 
			&WaylandDataSource::action, void(wl_data_source*, unsigned int)>,
	};

	wl_data_source_add_listener(wlDataSource_, &listener, this);

	auto types = source_->types();
	for(auto& t : types.types)
	{
		auto mimeType = formatToMimeType(t);
		if(!mimeType) continue;
		wl_data_source_offer(wlDataSource_, mimeType);
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
	debug("target ", mimeType);
}
void WaylandDataSource::send(const char* mimeType, int fd)
{
	auto fmt = mimeTypeToFormat(mimeType);
	auto data = source_->data(fmt);
	if(data.has_value())
	{
		std::vector<std::uint8_t> buffer;
		if(fmt == dataType::text)
		{
			auto text = std::any_cast<std::string&>(data);
			buffer = {text.begin(), text.end()};
		}
		else
		{
			buffer = std::move(std::any_cast<std::vector<std::uint8_t>&>(data));
		}

		write(fd, buffer.data(), buffer.size());
	}
	else
	{
		warning("WaylandDataSource::send: failed to retrieve data object.");
	}

	close(fd);
	debug("send ", mimeType, " fd: ", fd);
}

void WaylandDataSource::action(unsigned int action)
{
	debug("action ", action);

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

	appContext_.waylandMouseContext().cursorBuffer(buffer, hotspot, size);
}

void WaylandDataSource::dndPerformed()
{
	//we dont care about this information
	//important: do not destroy the source here, only in cancelled/finished
	debug("performed");
}

void WaylandDataSource::cancelled()
{
	//destroy self here
	debug("cancelled");
	delete this;
}

void WaylandDataSource::dndFinished()
{
	//destroy self here
	debug("finished");
	delete this;
}

//WaylandDataDevice
WaylandDataDevice::WaylandDataDevice(WaylandAppContext& ac) : appContext_(&ac)
{
	auto wldm = ac.wlDataManager();
	wlDataDevice_ = wl_data_device_manager_get_data_device(wldm, ac.wlSeat());

	static constexpr wl_data_device_listener listener =
	{
		memberCallback<decltype(&WaylandDataDevice::offer), 
			&WaylandDataDevice::offer, void(wl_data_device*, wl_data_offer*)>,

		memberCallback<decltype(&WaylandDataDevice::enter), 
			&WaylandDataDevice::enter, void(wl_data_device*, unsigned int, wl_surface*, 
			wl_fixed_t, wl_fixed_t, wl_data_offer*)>,

		memberCallback<decltype(&WaylandDataDevice::leave), 
			&WaylandDataDevice::leave, void(wl_data_device*)>,

		memberCallback<decltype(&WaylandDataDevice::motion), &WaylandDataDevice::motion, 
			void(wl_data_device*, unsigned int, wl_fixed_t, wl_fixed_t)>,

		memberCallback<decltype(&WaylandDataDevice::drop), 
			&WaylandDataDevice::drop, void(wl_data_device*)>,

		memberCallback<decltype(&WaylandDataDevice::selection), 
			&WaylandDataDevice::selection, void(wl_data_device*, wl_data_offer*)>,
	};

	wl_data_device_add_listener(wlDataDevice_, &listener, this);
}

WaylandDataDevice::~WaylandDataDevice()
{
	if(wlDataDevice_) wl_data_device_destroy(wlDataDevice_);
}

void WaylandDataDevice::offer(wl_data_offer* offer)
{
	debug("offer");
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
	debug("leave");
	if(dndOffer_)
	{
		offers_.erase(std::remove_if(offers_.begin(), offers_.end(), 
			[=](auto& v) { return v.get() == dndOffer_; }), offers_.end());

		dndOffer_ = nullptr;
	}
}

void WaylandDataDevice::motion(unsigned int time, wl_fixed_t x, wl_fixed_t y)
{
	debug("motion");
	nytl::unused(time, x, y);

	wl_data_offer_set_actions(&dndOffer_->wlDataOffer(), WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE,
		WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);
}

void WaylandDataDevice::drop()
{
	debug("drop");
	if(!dndOffer_)
	{
		warning("ny::WaylandDataDevice::drop: invalid current dnd session.");
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
		debug("drop drop");
		DataOfferEvent event(dndWC_->eventHandler());
		event.offer = std::move(ownedDndOffer);
		appContext_->dispatch(std::move(event));
	}

	dndOffer_ = nullptr;
}

void WaylandDataDevice::selection(wl_data_offer* offer)
{
	debug("selection");
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
