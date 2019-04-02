// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/wayland/util.hpp> // ny::wayland::ShmBuffer
#include <ny/dataExchange.hpp> // ny::DataOffer

#include <map> // std::map
#include <vector> // std::vector
#include <memory> // std::unique_ptr
#include <string> // std::string
#include <utility> // std::pair

namespace ny {

/// DataOffer implementation for the wayland backend and wrapper around wl_data_offer.
class WaylandDataOffer : public DataOffer, nytl::NonMovable {
public:
	WaylandDataOffer();
	WaylandDataOffer(WaylandAppContext& ac, wl_data_offer& wlDataOffer);
	~WaylandDataOffer();

	bool formats(FormatsListener) override;
	bool data(nytl::StringParam format, DataListener) override;
	void preferred(nytl::StringParam format, DndAction) override;

	DndAction action() override { return action_; }
	nytl::Flags<DndAction> supportedActions() override { return actions_; }

	wl_data_offer& wlDataOffer() const { return *wlDataOffer_; }
	WaylandAppContext& appContext() const { return *appContext_; }

	bool valid() const { return (wlDataOffer_); }

protected:
	WaylandAppContext* appContext_ {};
	wl_data_offer* wlDataOffer_ {};
	std::vector<std::string> formats_ {}; // supported formats by other side
	nytl::Flags<DndAction> actions_ {}; // supported actions by other side

	std::string accepted_ {}; // preferred format
	DndAction action_ {}; // target action as chosen by compositor

	// pending data request
	struct Request {
		nytl::UniqueConnection fdConnection; // listening for data on df
		std::vector<DataListener> listener; // listener to forward data to
		std::vector<std::byte> buffer; // already received data
	};

	// pending data requests
	std::unordered_map<std::string, Request> pending_;

protected:
	bool fdCallbackDnd(int fd, std::string);

	/// Wayland callback that is called everytime a new mimeType is announced.
	/// This might then trigger an onFormat callback.
	void offer(wl_data_offer*, const char* mimeType);

	/// Source actions are currently not implemented since they do not have an interface.
	void sourceActions(wl_data_offer*, uint32_t actions);

	/// Source actions are currently not implemented since they do not have an interface.
	void handleAction(wl_data_offer*, uint32_t action);

	/// This function is registered as callback function when a data receive fd can be
	/// read.
	void fdReceive(wl_data_offer*, int32_t fd);

	/// Called by destructor and move assignment operator
	void destroy();
};

/// Free wrapper class around wl_data_source objects.
/// Note that this class does always destroy itself when it is no longer needed by
/// the wayland compositor. It represents a wl_data_source implementation for a given
/// ny::DataSource implementation.
/// Therefore this object should not have an owner (which does make sense) and not
/// be wrapped in smart pointers such as shared_ptr or unique_ptr.
/// Leaks occur if an object of this class is created without ever being used as
/// relevant data source, i.e. never used as dnd or clipboard source.
class WaylandDataSource {
public:
	WaylandDataSource(WaylandAppContext&, std::unique_ptr<DataSource>&&, bool dnd);
	~WaylandDataSource();

	wl_data_source& wlDataSource() const { return *wlDataSource_; }
	DataSource& dataSource() const { return *source_; }

	bool dnd() const { return dnd_; }
	wl_surface* dragSurface() const { return dragSurface_; }

	/// Draws onto the dragSurface, i.e. attaches and commits a buffer.
	/// This extra function is needed since this can only be done after the
	/// dnd operation was started because before it the surface has no role.
	void drawSurface();

protected:
	WaylandAppContext& appContext_;
	std::unique_ptr<DataSource> source_;
	wl_data_source* wlDataSource_ {};
	bool dnd_ {}; // whether this is a dnd data source

	DndAction action_ {};
	std::string target_ {};

	wl_surface* dragSurface_ {};
	wayland::ShmBuffer dragBuffer_ {};

protected:
	void updateCursor();

	void target(wl_data_source*, const char* mimeType);
	void send(wl_data_source*, const char* mimeType, int32_t fd);
	void dndPerformed(wl_data_source*);
	void action(wl_data_source*, uint32_t action);

	void cancelled(wl_data_source*); /// Destroys itself
	void dndFinished(wl_data_source*); /// Destroys itself
};

/// Wrapper and implementation around wl_data_device.
/// Manages all introduces wl_data_offer objects and keeps track of the current
/// clipboard and dnd data offers if there are any.
class WaylandDataDevice {
public:
	WaylandDataDevice(WaylandAppContext&);
	~WaylandDataDevice();

	wl_data_device& wlDataDevice() const { return *wlDataDevice_; }

	unsigned dndSerial() const { return dnd_.serial; }
	WaylandDataOffer* clipboardOffer() const { return clipboardOffer_; }
	WaylandDataOffer* dndOffer() const { return dnd_.offer; }
	WaylandWindowContext* dndWC() const { return dnd_.wc; }

protected:
	WaylandAppContext* appContext_ {};
	wl_data_device* wlDataDevice_ {};

	std::vector<std::unique_ptr<WaylandDataOffer>> offers_;
	WaylandDataOffer* clipboardOffer_ {};

	// current dnd session
	struct {
		WaylandWindowContext* wc {};
		unsigned int serial {};
		nytl::Vec2i position {};
		WaylandDataOffer* offer {};
	} dnd_;

protected:
	decltype(offers_)::iterator findOffer(const WaylandDataOffer&);
	decltype(offers_)::iterator findOffer(const wl_data_offer&);

	/// Introduces and creates a new WaylandDataOffer object.
	void offer(wl_data_device*, wl_data_offer* offer);

	/// Sets the dndOffer to the given data offer
	void enter(wl_data_device*, uint32_t serial, wl_surface*, wl_fixed_t x,
		wl_fixed_t y, wl_data_offer*);

	/// Unsets the dndDataOfferand destroys the associated WaylandDataOffer object
	void leave(wl_data_device*);

	/// Updates the actions depending on the current position and dndWc
	void motion(wl_data_device*, uint32_t time, wl_fixed_t x, wl_fixed_t y);

	/// Sends out a DataOffer event to the handler of dndWc
	/// dndOfferis unset and the WaylandDataOffer moved to DataOfferEvent::offer
	void drop(wl_data_device*);

	/// updates clipboardOffer and destroyes the previous clipboard WaylandDataOffer
	void selection(wl_data_device*, wl_data_offer*);
};

} // namespace ny
