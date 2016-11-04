#pragma once

#include <ny/wayland/include.hpp>
#include <ny/data.hpp>

#include <nytl/callback.hpp>
#include <map>

namespace ny
{


///DataOffer implementation for the wayland backend and wrapper around wl_data_offer.
class WaylandDataOffer : public DataOffer
{
public:
	WaylandDataOffer() = default;
	WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer, bool dnd = false);
	~WaylandDataOffer();

	WaylandDataOffer(WaylandDataOffer&& other) noexcept;
	WaylandDataOffer& operator=(WaylandDataOffer&& other) noexcept;

	DataTypes types() const override { return dataTypes_; }
	nytl::Connection data(unsigned int fmt, const DataOffer::DataFunction& func) override;

	wl_data_offer& wlDataOffer() const { return *wlDataOffer_; }
	WaylandAppContext& appContext() const { return *appContext_; }

	bool dnd() const { return dnd_; }
	void dnd(bool x) { dnd_ = x; }

	bool valid() const { return (wlDataOffer_); }

protected:
	WaylandAppContext* appContext_ {};
	wl_data_offer* wlDataOffer_ {};
	DataTypes dataTypes_ {};
	bool dnd_ {};

	struct PendingRequest
	{
		std::vector<std::uint8_t> buffer;
		nytl::Callback<void(const std::any&, DataOffer&, unsigned int)> callback;
		nytl::ConnectionGuard connection;
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

	///Called by destructor and move assignment operator
	void destroy();
};

///Free wrapper class around wl_data_source objects.
///Note that this class does always destroy itself when it is no longer needed by
///the wayland compositor. It represents a wl_data_source implementation for a given
///ny::DataSource implementation.
///Therefore this object should not have an owner (which does make sense) and not
///be wrapped in smart pointers such as shared_ptr or unique_ptr.
///Leaks occur if an object of this class is created without ever being used as
///relevant data source, i.e. never used as dnd or clipboard source.
class WaylandDataSource
{
public:
	WaylandDataSource(WaylandAppContext&, std::unique_ptr<DataSource>, bool dnd);
	
	wl_data_source& wlDataSource() const { return *wlDataSource_; }
	DataSource& dataSource() const { return *source_; }
	bool dnd() const { return dnd_; }

protected:
	WaylandAppContext& appContext_;
	std::unique_ptr<DataSource> source_;
	wl_data_source* wlDataSource_ {};
	bool dnd_ {};

protected:
	~WaylandDataSource();

	void target(const char* mimeType);
	void send(const char* mimeType, int fd);
	void dndPerformed();
	void action(unsigned int action);

	void cancelled(); ///Destroys itself
	void dndFinished(); ///Destroys itself
};

///Wrapper and implementation around wl_data_device.
///Manages all introduces wl_data_offer objects and keeps track of the current
///clipboard and dnd data offers if there are any.
class WaylandDataDevice
{
public:
	WaylandDataDevice(WaylandAppContext&);
	~WaylandDataDevice();

	wl_data_device& wlDataDevice() const { return *wlDataDevice_; }

	WaylandDataOffer* clipboardOffer() const { return clipboardOffer_; }
	WaylandDataOffer* dndOffer() const { return dndOffer_; }
	WaylandWindowContext* dndWC() const { return dndWC_; }

protected:
	WaylandAppContext* appContext_ {};
	wl_data_device* wlDataDevice_ {};

	std::vector<std::unique_ptr<WaylandDataOffer>> offers_; //TODO: do it without dma/pointers

	WaylandDataOffer* clipboardOffer_ {};	
	WaylandDataOffer* dndOffer_ {};	
	
	WaylandWindowContext* dndWC_;

protected:
	///Introduces and creates a new WaylandDataOffer object.
	void offer(wl_data_offer* offer);

	///Sets the dndOffer to the given data offer
	void enter(unsigned int serial, wl_surface*, wl_fixed_t x, wl_fixed_t y, wl_data_offer*);

	///Unsets the dndDataOfferand destroys the associated WaylandDataOffer object
	void leave();

	///Updates the actions depending on the current position and dndWc
	void motion(unsigned int time, wl_fixed_t x, wl_fixed_t y);

	///Sends out a DataOffer event to the handler of dndWc
	///dndOfferis unset and the WaylandDataOffer moved to DataOfferEvent::offer
	void drop();

	///updates clipboardOffer and destroyes the previous clipboard WaylandDataOffer
	void selection(wl_data_offer*);
};

}
