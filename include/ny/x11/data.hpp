#pragma once

#include <ny/data.hpp>
#include <xcb/xcb.h>
#include <memory>

namespace ny
{

//TODO: something about notify event timeout

class X11DataOffer : public DataOffer
{
public:
	X11DataOffer() = default;
	X11DataOffer(unsigned int selection);
	~X11DataOffer() = default;

	DataTypes types() const override;
	nytl::Connection data(unsigned int fmt, const DataFunction& func) override;

	//x11 specific
	void notify(const xcb_selection_notify_event_t& notify);

	xcb_atom_t selection() const { return selection_; }
	xcb_window_t owner() const { return owner_; }
	bool valid() const { return (selection_); }

protected:
	xcb_atom_t selection_ {};
	xcb_window_t owner_ {};
	DataTypes types_;
};


///Manages all selection, Xdnd and data exchange interactions.
class X11DataManager
{
public:
	X11DataManager() = default;
	X11DataManager(X11AppContext& ac);
	~X11DataManager() = default;

	bool handleEvent(xcb_generic_event_t& event);

	X11AppContext& appContext() const { return *appContext_; }
	xcb_connection_t& xConnection() const;
	xcb_window_t dummyWindow() const;

protected:
	void answerRequest(DataSource& source, const xcb_selection_request_event_t& request);

	std::vector<xcb_atom_t> formatToTargetAtom(unsigned int format);
	unsigned int targetAtomToFormat(xcb_atom_t atom);

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
