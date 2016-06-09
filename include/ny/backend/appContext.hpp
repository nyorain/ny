#pragma once

#include <ny/include.hpp>
#include <nytl/nonCopyable.hpp>
#include <memory>

namespace ny
{

///Abstract base interface for a backend-specific display conncetion.
///Defines the interfaces for different (threaded/blocking) event dispatching functions
///that have to be implemented by the different backends.
class AppContext : public NonCopyable
{
public:
    AppContext() = default;
    virtual ~AppContext() = default;

	///Dispatches all retrieved events to the registered handlers using the given EventDispatcher.
	///Does only dispatch all currently queued events and does not wait/block for new events.
	///This function is not threadsafe, i.e. it is undefined what happens it other threads
	///use the given EventDispatcher until the function returns.
	///\return false if the display conncetion was destroyed (or an error occurred),
	///true otherwise (if all queued events were dispatched).
	virtual bool dispatchEvents(EventDispatcher& dispatcher) = 0;

	///Blocks and dispatches all incoming display events until the stop function of the loop control
	///is called or the display conncection is closed by the server (e.g. an error or exit event).
	///\param control A AppContext::LoopControl object that can be used to stop the loop from
	///inside or from another thread.
	///\return true if loop was exited because stop() was called by the LoopControl.
	///\return false if loop was exited because the display conncetion was destroyed.
	virtual bool dispatchLoop(EventDispatcher& dispatcher, LoopControl& control) = 0;

	///Blocks ans dispatches all incoming events from the display and queued events inside
	///the EventDispatcher until the stop function of the given loop control is called or
	///the display connection is closed by the server.
	///This function only accepts an ThreadedEventDispatcher as dispatcher paramter and
	///guarantess furthermore threadsafety, i.e. events from other threads dispatched over the
	///given EventDispatcher will be sent.
	////This function itself will take care of dispatching all events, so the dispatcher
	///loop of the given dispatcher should not be run in different threads.
	///\sa ThreadedEventDispatcher
	///\param control A AppContext::LoopControl object that can be used to stop the loop from
	///inside or from another thread.
	///\return true if loop was exited because stop() was called by the LoopControl.
	///\return false if loop was exited because the display conncetion was destroyed.
	virtual bool threadedDispatchLoop(ThreadedEventDispatcher& dispatcher, LoopControl& control) = 0;
};

}
