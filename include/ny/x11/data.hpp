// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/data.hpp>
#include <xcb/xcb.h>
#include <memory>
#include <map>

namespace ny
{

///Converts a given dataType format to a list of target atoms that represent the given
//format. If none atoms are found, an empty vector is returned.
std::vector<xcb_atom_t> formatToTargetAtom(const x11::Atoms&, unsigned int format);

///Converts a given target atom to the dataType format representing it.
///Returns dataType::none for unknown atoms.
unsigned int targetAtomToFormat(const x11::Atoms&, xcb_atom_t atom);

///X11 DataOffer implementation for selections or dnd.
class X11DataOffer : public DataOffer
{
public:
	X11DataOffer() = default;
	X11DataOffer(X11AppContext&, unsigned int selection, xcb_window_t owner);
	~X11DataOffer() = default;

	//can they be defaulted?
	X11DataOffer(X11DataOffer&&) = default;
	X11DataOffer& operator=(X11DataOffer&&) = default;

	DataTypes types() const override;
	nytl::Connection data(unsigned int fmt, const DataFunction& func) override;

	//x11 specific
	///Handle a recevied xcb_selection_notify_event.
	void notify(const xcb_selection_notify_event_t& notify);

	X11AppContext& appContext() const { return *appContext_; }
	xcb_atom_t selection() const { return selection_; }
	xcb_window_t owner() const { return owner_; }
	bool valid() const { return (selection_); }

protected:
	X11AppContext* appContext_ {};
	xcb_atom_t selection_ {};
	xcb_window_t owner_ {};

	//PendingRequest struct with target/format cache?
	using RequestCallback = nytl::Callback<void(const std::any&, DataOffer&, unsigned int)>;
	std::map<xcb_atom_t, RequestCallback> requests_;
	std::vector<std::pair<unsigned int, xcb_atom_t>> dataTypes_;
};


///Manages all selection, Xdnd and data exchange interactions.
///The dataSource pointer members should only have a value as long as the
///application has ownership over the associated selection.
class X11DataManager
{
public:
	X11DataManager() = default;
	X11DataManager(X11AppContext& ac);
	~X11DataManager() = default;

	bool handleEvent(xcb_generic_event_t& event);

	X11AppContext& appContext() const { return *appContext_; }
	xcb_connection_t& xConnection() const;
	xcb_window_t xDummyWindow() const;
	const x11::Atoms& atoms() const;

	bool clipboard(std::unique_ptr<DataSource>&&);
	DataOffer* clipboard();

protected:
	void answerRequest(DataSource& source, const xcb_selection_request_event_t& request);
	xcb_window_t selectionOwner(xcb_atom_t selection);

protected:
	X11AppContext* appContext_;

	std::unique_ptr<DataSource> clipboardSource_;
	std::unique_ptr<DataSource> primarySource_;
	std::unique_ptr<DataSource> dndSource_;

	X11DataOffer clipboardOffer_;
	X11DataOffer primaryOffer_;
	X11DataOffer currentDndOffer_; //the currently active data offer
	std::vector<X11DataOffer*> dndOffers_; //old data offers and the currently actvie one
};

}
