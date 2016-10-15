#include <ny/backend/wayland/data.hpp>
#include <ny/base/log.hpp>

#include <nytl/stringParam.hpp>

#include <wayland-client-protocol.h>

#include <unistd.h>
#include <fcntl.h>

namespace ny
{

namespace
{
	//TODO: could be part of the public interface
	//TODO: still many missing
	unsigned int mimeTypeToFormat(nytl::StringParam param)
	{
		if(param == "text/plain") return dataType::text;

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
}

WaylandDataOffer::~WaylandDataOffer()
{
	wl_data_offer_finish(wlDataOffer_);
	if(wlDataOffer_) wl_data_offer_destroy(wlDataOffer_);
}

nytl::CbConn WaylandDataOffer::data(unsigned int fmt, const DataFunction& func)
{
	auto mimeType = formatToMimeType(fmt);
	if(!mimeType)
	{
		warning("WaylandDataOffer::data: unsupported format.");
		func(*this, fmt, {});
		return {};
	}

	int fds[2];
	auto ret = pipe2(fds, O_CLOEXEC);
	wl_data_offer_receive(wlDataOffer_, mimeType, fds[1]);

	return dataCallbacks_[fmt].add(func);
}

void WaylandDataOffer::offer(const char* mimeType)
{
	auto fmt = mimeTypeToFormat(mimeType);
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
