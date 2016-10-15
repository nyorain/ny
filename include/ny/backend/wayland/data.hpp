#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/base/data.hpp>
#include <nytl/callback.hpp>
#include <map>

namespace ny
{

///DataOffer implementation for the wayland backend.
class WaylandDataOffer : public DataOffer
{
public:
	WaylandDataOffer() = default;
	WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer);
	~WaylandDataOffer();

	WaylandDataOffer(WaylandDataOffer&& other) noexcept;
	WaylandDataOffer& operator=(WaylandDataOffer&& other) noexcept;

	DataTypes types() const override { return dataTypes_; }
	nytl::CbConn data(unsigned int fmt, const DataFunction& func) override;

	wl_data_offer& wlDataOffer() const { return *wlDataOffer_; }
	WaylandAppContext& appContext() const { return *appContext_; }

	bool valid() const { return (wlDataOffer_); }

protected:
	WaylandAppContext* appContext_ {};
	wl_data_offer* wlDataOffer_ {};
	DataTypes dataTypes_ {};

	std::map<unsigned int, nytl::Callback<void(unsigned int, std::any)>> dataCallbacks_;

protected:
	///Wayland callback that is called everytime a new mimeType is announced.
	///This might then trigger an onFormat callback.
	void offer(const char* mimeType);

	///Source actions are currently not implemented since they do not have an interface.
	void sourceActions(unsigned int actions);

	///Source actions are currently not implemented since they do not have an interface.
	void action(unsigned int action);
};

}
