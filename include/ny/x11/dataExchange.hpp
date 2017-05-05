// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/dataExchange.hpp>
#include <nytl/nonCopyable.hpp>

//TODO: delete this header pull, use x11::GenericEvent for event functions
#include <xcb/xcb.h>

#include <memory>
#include <map>
#include <unordered_map>

namespace ny {

/// X11 DataOffer implementation for selections or dnd.
/// Manage all communication with the owner of the associated selection and handles
/// all asynchronous requests.
/// Note that this is by far the most complex DataOffer implementation since the X11
/// backends handles the formats as well as all data requests in an asynchronous manner.
class X11DataOffer : public DataOffer, public nytl::NonMovable {
public:
	template<typename T> class AsyncRequestImpl;
	class DataFormatRequestImpl;

public:
	/// Constructs the DataOffer without the supported targets.
	/// Will request them in the constructor.
	X11DataOffer(X11AppContext&, unsigned int selection, xcb_window_t owner);

	/// Construct the DataOffer with the known supported target formats.
	/// This constructor will store them and remember that they don't have to
	/// requested anymore.
	X11DataOffer(X11AppContext&, unsigned int selection, xcb_window_t owner,
		nytl::Span<const xcb_atom_t> supportedTargets);

	~X11DataOffer();

	// - DataOffer implementation -
	FormatsRequest formats() override;
	DataRequest data(const DataFormat& format) override;

	// - x11 specific -
	/// Handle a recevied xcb_selection_notify_event for the selection represented
	/// by this offer.
	void notify(const xcb_selection_notify_event_t& notify);

	/// Signals the DataOffer that its ownership will be passed to the application and that
	/// it therefore has to unregister itself from the DataManager on destruction.
	void unregister() { unregister_ = true; }

	X11AppContext& appContext() const { return *appContext_; }
	xcb_atom_t selection() const { return selection_; } //the associated x selection
	xcb_window_t owner() const { return owner_; } //the owner of the selection

protected:
	/// This will register a data request for the given format.
	/// If the given format is not supported, the request is completed with an empty any object
	/// and en empty connection will be returned.
	/// Otherwise the connection guard for a callback into the given request that will
	/// be triggered when the requested data is received will be returned.
	nytl::UniqueConnection registerDataRequest(const DataFormat& format,
		AsyncRequestImpl<std::any>& request);

	/// Converts and adds the given target atom format to the supported formats.
	void addFormats(nytl::Span<const xcb_atom_t> targets);

protected:
	X11AppContext* appContext_ {};
	xcb_atom_t selection_ {};
	xcb_window_t owner_ {};

	// TODO: also store a vector of all supported DataFormats (extracted from formats_)
	//   so they don't have to be extracted in every formats request
	std::unordered_map<DataFormat, xcb_atom_t> formats_;
	bool formatsRetrieved_ {};
	bool unregister_ {};

	// callbacks into the pending AsyncRequest objects that are not yet completed
	nytl::Callback<void(std::vector<DataFormat>)> pendingFormatRequests_;
	std::map<xcb_atom_t, nytl::Callback<void(std::any)>> pendingDataRequests_;
};

/// Implements the source site of an X11 selection.
/// Always owns the applicatoins DataSource implementation and represents
/// it as selection for other x clients.
class X11DataSource : public nytl::NonCopyable {
public:
	X11DataSource() = default;
	X11DataSource(X11AppContext&, std::unique_ptr<DataSource> src);
	~X11DataSource() = default;

	X11DataSource(X11DataSource&&) noexcept = default;
	X11DataSource& operator=(X11DataSource&&) noexcept = default;

	X11AppContext& appContext() const { return *appContext_; }
	DataSource& dataSource() const { return *dataSource_; }
	bool valid() const { return dataSource_.get(); }
	const std::vector<xcb_atom_t>& targets() const { return targets_; }

	/// Will answer the received convert selection request, i.e. by sending all
	/// supported targets or trying to send the data from the source in the requested format.
	void answerRequest(const xcb_selection_request_event_t& requestEvent);

protected:
	X11AppContext* appContext_;
	std::unique_ptr<DataSource> dataSource_;

	// TODO: also store a vector of all supported xcb_atoms (extracted from formats_)
	//   so they don't have to be extracted in every target convert selection
	std::vector<std::pair<xcb_atom_t, DataFormat>> formatsMap_;
	std::vector<xcb_atom_t> targets_;
};

/// Manages all selection, Xdnd and data exchange interactions.
/// The dataSource pointer members should only have a value as long as the
/// application has ownership over the associated selection.
class X11DataManager : public nytl::NonCopyable {
public:
	X11DataManager() = default;
	X11DataManager(X11AppContext& ac);
	~X11DataManager() = default;

	X11DataManager(X11DataManager&&) noexcept = default;
	X11DataManager& operator=(X11DataManager&&) noexcept = default;

	X11AppContext& appContext() const { return *appContext_; }
	xcb_connection_t& xConnection() const;
	xcb_window_t xDummyWindow() const;
	const x11::Atoms& atoms() const;

	/// Tries to handle the given event. Return true if it was handled.
	bool processEvent(const xcb_generic_event_t& event);

	/// Tries to claim clipboard ownership and set it to the given DataSource.
	/// Returns true on success and false on failure.
	bool clipboard(std::unique_ptr<DataSource>&&);

	/// Returns the DataOffer for the current clipboard.
	/// Returns a nullptr if there is no current clipboard selection owner or it
	/// could not be received. The returned pointer is guaranteed to be valid until the next
	/// time this function or a dispatch function of the associated AppContext is called.
	DataOffer* clipboard();

	/// Starts a drag and drop session for the given DataSource implementation.
	/// Returns whether the operation succeeded.
	/// Returns immidietly, i.e. does not wait for the dnd session to end.
	bool startDragDrop(std::unique_ptr<DataSource>);

	/// Called from within the X11DataOffer destructor when ownership for the DataOffer
	/// was passed to the application. Signals the DataManager that no further notify
	/// events should be dispatched to the DataOffer.
	void unregisterDataOffer(const X11DataOffer&);

protected:
	/// Returns the owner of the given selection atom.
	/// When selection is e.g. the clipboard atom (appContext().atoms().clipboard), this will
	/// return the window that holds clipboard ownership.
	/// If the selection is unknown/not supported or there is no owner returns 0.
	xcb_window_t selectionOwner(xcb_atom_t selection);

	/// Tries to handle the given client message.
	/// Returns true if it was handled.
	bool processClientMessage(const xcb_client_message_event_t&, const EventData&);

	/// Tries to handle the given event if currently implementing a dnd session.
	/// Returns true if it was handled.
	bool processDndEvent(const xcb_generic_event_t& ev);

	void xdndSendEnter();
	void xdndSendLeave();
	void xdndSendPosition(nytl::Vec2i rootPosition);

protected:
	X11AppContext* appContext_;

	X11DataSource clipboardSource_; //CLIPBOARD atom selection
	X11DataSource primarySource_; //PRIMARY atom selection

	std::unique_ptr<X11DataOffer> clipboardOffer_;
	std::unique_ptr<X11DataOffer> primaryOffer_;

	// we have to stricly seperate dnd src and offer since we might be source
	// and target at the same time.
	// values for the current dnd session as source
	struct {
		unsigned int version {}; //protocol version with the window we are currently over
		xcb_window_t sourceWindow {}; //the source window in this application
		xcb_window_t targetWindow {}; //the dnd aware window we are currently over
		X11DataSource source {};
	} dndSrc_;

	// values for the current dnd session as target
	struct {
		unsigned int version {}; //xdnd version
		X11WindowContext* windowContext {}; //the window context over which the offer is
		std::unique_ptr<X11DataOffer> offer; //the currently active data offer
	} dndOffer_;


	// old data offers and the currently active one
	// old data offers whose ownership has been passed to the application must be stored
	// since notify events for them must be dispatched correctly nontheless.
	// they unregister themself here calling unregisterDataOffer.
	std::vector<X11DataOffer*> dndOffers_;

	//TODO: store and dispatch to old dnd sources as well?
};

} // namespace ny
