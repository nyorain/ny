#include <ny/backend/wayland/data.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/base/log.hpp>

#include <nytl/stringParam.hpp>

#include <wayland-client-protocol.h>

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>

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
			case dataType::text: return "text/plain";
			case dataType::png: return "image/png";
			case dataType::bmp: return "image/bmp";
			case dataType::jpeg: return "image/jpeg";
			case dataType::gif: return "image/gif";
			case dataType::mp3: return "image/mpeg";
		}

		return nullptr;
	}

}

WaylandDataOffer::WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer)
	: appContext_(&ac), wlDataOffer_(&wlDataOffer)
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

WaylandDataOffer::WaylandDataOffer(WaylandDataOffer&& other) noexcept
	: appContext_(other.appContext_), wlDataOffer_(other.wlDataOffer_), 
	dataTypes_(std::move(other.dataTypes_)), requests_(std::move(other.requests_))
{
	if(wlDataOffer_) wl_data_offer_set_user_data(wlDataOffer_, this);

	other.appContext_ = {};
	other.wlDataOffer_ = {};
}

WaylandDataOffer& WaylandDataOffer::operator=(WaylandDataOffer&& other) noexcept
{
	if(wlDataOffer_)
	{
		wl_data_offer_finish(wlDataOffer_);
		wl_data_offer_destroy(wlDataOffer_);
	}

	appContext_ = other.appContext_;
	wlDataOffer_ = other.wlDataOffer_;
	dataTypes_ = std::move(other.dataTypes_);
	requests_ = std::move(other.requests_);

	if(wlDataOffer_) wl_data_offer_set_user_data(wlDataOffer_, this);

	return *this;
}

WaylandDataOffer::~WaylandDataOffer()
{
	if(wlDataOffer_)
	{
		// wl_data_offer_finish(wlDataOffer_);
		wl_data_offer_destroy(wlDataOffer_);
	}
}

nytl::CbConn WaylandDataOffer::data(DataOffer& offer, unsigned int fmt, 
	const DataOffer::DataFunction& func)
{
	auto mimeType = formatToMimeType(fmt);
	if(!mimeType)
	{
		warning("WaylandDataOffer::data: unsupported format.");
		func({}, offer, fmt);
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
			func({}, offer, fmt);
			return {};
		}

		wl_data_offer_receive(wlDataOffer_, mimeType, fds[1]);

		auto callback = [wlOffer = wlDataOffer_, ac = appContext_, fmt, &offer](int fd) 
		{
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

			self->requests_[fmt].callback(any, offer, fmt);
			self->requests_[fmt].callback.clear();
			self->requests_[fmt].connection.destroy();
			self->requests_[fmt].buffer.clear();
			close(fd);
		};
		
		conn = appContext_->fdCallback(fds[0], POLLIN, callback);
	}

	return requests_[fmt].callback.add(func);
}

void WaylandDataOffer::offer(const char* mimeType)
{
	auto fmt = mimeTypeToFormat(mimeType);
	log("ny::WaylandDataOffer::offer: ", mimeType);
	log("ny::WaylandDataOffer::offer format: ", fmt);
	if(fmt != dataType::none) dataTypes_.add(fmt);
}

void WaylandDataOffer::sourceActions(unsigned int actions)
{
	nytl::unused(actions);
}

void WaylandDataOffer::action(unsigned int action)
{
	nytl::unused(action);
}

}
