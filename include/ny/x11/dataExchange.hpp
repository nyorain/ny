// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/dataExchange.hpp>
#include <nytl/nonCopyable.hpp>

#include <memory>
#include <map>
#include <unordered_map>

namespace ny {

/// X11 DataOffer implementation for selections or dnd.
/// Manage all communication with the owner of the associated selection and handles
/// all asynchronous requests.
/// Note that this is the most complex DataOffer implementation since the X11
/// backends handles the formats as well as all data requests in an asynchronous manner.
class X11DataOffer : public DataOffer, public nytl::NonMovable {
public:
	/// Constructs the DataOffer without the supported targets.
	/// Will request them in the constructor.
	X11DataOffer(X11AppContext&, unsigned int selection, xcb_window_t owner,
		xcb_timestamp_t time);

	/// Constructor for xdnd offers.
	/// Construct the DataOffer with the known supported target formats.
	/// This constructor will store them and remember that they don't have to
	/// requested anymore.
	X11DataOffer(X11AppContext&, unsigned int selection, xcb_window_t owner,
		nytl::Span<const xcb_atom_t> supportedTargets, unsigned xdndVersion,
		xcb_timestamp_t time);

	~X11DataOffer();

	// - DataOffer implementation -
	bool formats(FormatsListener) override;
	bool data(nytl::StringParam format, DataListener) override;

	// dnd
	void preferred(nytl::StringParam format,
		DndAction action = DndAction::copy) override;
	DndAction action() override { return action_; }
	nytl::Flags<DndAction> supportedActions() override {
		return action_ | DndAction::copy;
	}

	// - x11 specific -
	void action(DndAction action) { action_ = action; }
	void timestamp(xcb_timestamp_t time) { lastTime_ = time; }
	void owner(xcb_window_t win) { owner_ = win; }
	void over(xcb_window_t win) { over_ = win; }

	/// Handle a recevied xcb_selection_notify_event for the selection represented
	/// by this offer. Returns false if the notification is not for
	/// any pending request by this offer.
	bool notify(const x11::GenericEvent& notify);

	/// Signals the DataOffer that its ownership will be passed to the application and that
	/// it therefore has to unregister itself from the DataManager on destruction.
	void finishDnd() { finishDnd_ = true; }

	X11AppContext& appContext() const { return *appContext_; }
	xcb_atom_t selection() const { return selection_; } // selection type
	xcb_window_t owner() const { return owner_; } // the owner of the selection
	unsigned xdndVersion() const { return xdndVersion_; }

protected:
	/// This will register a data request for the given format.
	/// If the given format is not supported, false is returned and the
	/// given listener not moved.
	bool addDataListener(nytl::StringParam format, DataListener&&);

	/// Converts and adds the given target atom format to the supported formats.
	void setFormats(nytl::Span<const xcb_atom_t> targets);

protected:
	X11AppContext* appContext_ {};
	xcb_atom_t selection_ {};
	xcb_window_t owner_ {};

	std::unordered_map<std::string, xcb_atom_t> formats_;
	bool formatsRetrieved_ {};
	xcb_timestamp_t lastTime_ {};

	// dnd
	bool finishDnd_ {};
	unsigned xdndVersion_ {};
	DndAction action_ {DndAction::copy};
	xcb_window_t over_ {};

	struct DataReq {
		xcb_timestamp_t time {};
		xcb_atom_t target {};
		std::vector<DataListener> listeners;
		std::string format;
	};

	struct {
		xcb_timestamp_t time {};
		std::vector<FormatsListener> listeners;
	} formatReq_;

	std::vector<DataReq> dataReqs_;
};

/// Implements the source site of an X11 selection.
/// Always owns the applicatoins DataSource implementation and represents
/// it as selection for other x clients.
class X11DataSource : public nytl::NonCopyable {
public:
	X11DataSource() = default;
	X11DataSource(X11AppContext&, std::unique_ptr<DataSource> src,
		xcb_timestamp_t acquired);
	~X11DataSource() = default;

	X11DataSource(X11DataSource&&) noexcept = default;
	X11DataSource& operator=(X11DataSource&&) noexcept = default;

	X11AppContext& appContext() const { return *appContext_; }
	DataSource& dataSource() const { return *dataSource_; }
	bool valid() const { return dataSource_.get(); }
	const std::vector<xcb_atom_t>& targets() const { return targets_; }
	xcb_timestamp_t acquired() const { return acquired_; }

	/// Will answer the received xcb_selection_request_event_t, i.e. by sending all
	/// supported targets or trying to send the data from the source in the requested format.
	void answerRequest(const x11::GenericEvent& requestEvent);

protected:
	X11AppContext* appContext_;
	std::unique_ptr<DataSource> dataSource_;
	xcb_timestamp_t acquired_ {};

	std::vector<std::pair<xcb_atom_t, std::string>> formatsMap_;
	std::vector<xcb_atom_t> targets_; // extracted from formatsMap_
};

/// Manages all selection, Xdnd and data exchange interactions.
/// The dataSource pointer members should only have a value as long as the
/// application has ownership over the associated selection.
class X11DataManager : public nytl::NonCopyable {
public:
	X11DataManager() = default;
	X11DataManager(X11AppContext& ac);
	~X11DataManager();

	X11DataManager(X11DataManager&&) noexcept = default;
	X11DataManager& operator=(X11DataManager&&) noexcept = default;

	X11AppContext& appContext() const { return *appContext_; }
	xcb_connection_t& xConnection() const;
	xcb_window_t xDummyWindow() const;
	const x11::Atoms& atoms() const;

	/// Tries to handle the given event. Return true if it was handled.
	bool processEvent(const x11::GenericEvent& event);

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
	bool startDragDrop(const EventData*, std::unique_ptr<DataSource>);

	/// Called from within the X11DataOffer destructor when ownership for the DataOffer
	/// was passed to the application. Signals the DataManager that no further notify
	/// events should be dispatched to the DataOffer.
	void unregisterDataOffer(const X11DataOffer&);

	/// Called from AppContext when the window context was destroyed.
	/// Cancels pending dnd requests.
	void destroyed(const X11WindowContext&);

protected:
	/// Returns the owner of the given selection atom.
	/// When selection is e.g. the clipboard atom (appContext().atoms().clipboard), this will
	/// return the window that holds clipboard ownership.
	/// If the selection is unknown/not supported or there is no owner returns 0.
	xcb_window_t selectionOwner(xcb_atom_t selection);

	/// Tries to handle the given xcb_client_message_event_t
	/// Returns true if it was handled.
	bool processClientMessage(const x11::GenericEvent&, const EventData&);

	/// Tries to handle the given event if currently implementing a dnd session.
	/// Returns true if it was handled.
	bool processDndEvent(const x11::GenericEvent& ev);

	void initCursors();
	void xdndSendEnter();
	void xdndSendLeave();
	void xdndSendPosition(nytl::Vec2i rootPosition, xcb_timestamp_t time);

protected:
	X11AppContext* appContext_;

	X11DataSource clipboardSource_; // CLIPBOARD atom selection
	X11DataSource primarySource_; // PRIMARY atom selection

	std::unique_ptr<X11DataOffer> clipboardOffer_;
	std::unique_ptr<X11DataOffer> primaryOffer_;

	// we have to stricly seperate dnd src and offer since we might be source
	// and target at the same time.
	// values for the current dnd session as source
	struct {
		unsigned int version {}; // protocol version with the window we are currently over
		xcb_window_t sourceWindow {}; // the source window in this application
		xcb_window_t targetWindow {}; // the dnd aware window we are currently over
		X11DataSource source {};
		std::unique_ptr<X11BufferWindowContext> dndWindow {};

		bool pendingStatus_ {}; // we expect a xdndStatus message
		// send xdndPosition with the stored values as soon as we receive
		// xdndStatus
		std::optional<std::pair<nytl::Vec2i, xcb_timestamp_t>> sendPos {};
	} dndSrc_;

	// values for the current dnd session as target
	struct {
		X11WindowContext* windowContext {}; // the window context over which the offer is
		std::unique_ptr<X11DataOffer> offer {}; // the currently active data offer
	} dndOffer_;

	struct {
		bool init {};
		xcb_cursor_t dndMove {};
		xcb_cursor_t dndCopy {};
		xcb_cursor_t dndNo {};
	} cursors_;

	// old data offers and the currently active one
	// old data offers whose ownership has been passed to the application must be stored
	// since notify events for them must be dispatched correctly nontheless.
	// they unregister themself here calling unregisterDataOffer.
	std::vector<X11DataOffer*> oldDndOffers_;

	// TODO:
	// stored as long as we have not received xdndfinished yet
	// the application might have started a new dnd though
	// (stored in reverse order; newer ones in the beginning)
	// std::vector<X11DataSource> oldDndSources_;
};

} // namespace ny
