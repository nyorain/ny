#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/base/data.hpp>
#include <nytl/callback.hpp>
#include <map>

namespace ny
{

///DataOffer implementation for the wayland backend.
class WaylandDataOffer 
{
public:
	WaylandDataOffer() = default;
	WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer);
	~WaylandDataOffer();

	WaylandDataOffer(WaylandDataOffer&& other) noexcept;
	WaylandDataOffer& operator=(WaylandDataOffer&& other) noexcept;

	DataTypes types() const { return dataTypes_; }
	nytl::CbConn data(DataOffer& offer, unsigned int fmt, const DataOffer::DataFunction& func);

	wl_data_offer& wlDataOffer() const { return *wlDataOffer_; }
	WaylandAppContext& appContext() const { return *appContext_; }

	bool valid() const { return (wlDataOffer_); }

protected:
	WaylandAppContext* appContext_ {};
	wl_data_offer* wlDataOffer_ {};
	DataTypes dataTypes_ {};

	struct PendingRequest
	{
		std::vector<std::uint8_t> buffer;
		nytl::Callback<void(const std::any&, DataOffer&, unsigned int)> callback;
		nytl::CbConnGuard connection;
	};

	std::map<unsigned int, PendingRequest> requests_;

protected:
	///Wayland callback that is called everytime a new mimeType is announced.
	///This might then trigger an onFormat callback.
	void offer(const char* mimeType);

	///Source actions are currently not implemented since they do not have an interface.
	void sourceActions(unsigned int actions);

	///Source actions are currently not implemented since they do not have an interface.
	void action(unsigned int action);

	///This function is registered as callback function when a data receive fd can be 
	///read. 
	void fdReceive(int fd);
};

class WaylandDataOfferWrapper : public DataOffer
{
public:
	WaylandDataOfferWrapper(WaylandDataOffer& offer) : offer_(&offer) {}

	DataTypes types() const { return offer_->types(); }
	nytl::CbConn data(unsigned int fmt, const DataOffer::DataFunction& func)
		{ return offer_->data(*this, fmt, func); }

protected:
	WaylandDataOffer* offer_ {};
};

}
