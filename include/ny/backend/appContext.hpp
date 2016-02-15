#pragma once

#include <ny/include.hpp>
#include <nytl/nonCopyable.hpp>
#include <memory>

namespace ny
{

///Abstract base interface for a backend-specific display conncetion.
class AppContext : public NonCopyable
{
public:
    AppContext() = default;
    virtual ~AppContext() = default;

	///Dispatches all retrieved events to the registered handlers using the given EventDispatcher.
	///\return false if the display conncetion was destroyed, true otherwise (normally).
	virtual bool dispatchEvents(EventDispatcher& dispatcher) = 0;

	///Blocks and dispatches all incoming events.
	///\param control A AppContext::LoopControl object that can be used to stop the loop from inside.
	///\return true if loop was exited because stop() was called by the LoopControl.
	///\return false if loop was exited because the display conncetion was destroyed.
	virtual bool dispatchLoop(EventDispatcher& dispatcher, LoopControl& control) = 0;

    //data
	/*
	virtual void startDataOffer(){}
    virtual bool isOffering() const { return 0; }
    virtual void endDataOffer(){}

    virtual dataOffer* getClipboard(){ return nullptr; };
    virtual void clipboard(dataSource& source, const event* ev){}

    //data specifications
    void clipboard(const std::string& str);
    void clipboard(const image& str);
	*/
};

}
