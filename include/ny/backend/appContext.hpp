#pragma once

#include <ny/include.hpp>
#include <nytl/nonCopyable.hpp>

namespace ny
{

class AppContext : public NonCopyable
{
public:
    AppContext() = default;
    virtual ~AppContext() = default;

	virtual int mainLoop() = 0;
	virtual void exit() {}

	///Dispatches all retrieved events to the registered handlers using the given EventDispatcher.
	///\return false if the display conncetion was destroyed, true otherwise (normally).
	virtual bool dispatchEvents(EventDispatcher& dispatcher) = 0;

    //data
	/*
    virtual void startDataOffer(dataSource& source, const image& img, const window& w, const event* ev){}
    virtual bool isOffering() const { return 0; }
    virtual void endDataOffer(){}

    virtual dataOffer* getClipboard(){ return nullptr; };
    virtual void setClipboard(dataSource& source, const event* ev){}

    //data specifications
    void setClipboard(const std::string& str);
    void setClipboard(const image& str);
	*/
};

}
