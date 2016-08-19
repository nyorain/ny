#pragma once

#include <ny/include.hpp>
#include <nytl/nonCopyable.hpp>
#include <memory>

///Header can be used without linking to ny-backend.

namespace ny
{

using WindowContextPtr = std::unique_ptr<WindowContext>;
using AppContextPtr = std::unique_ptr<AppContext>;

//TODO: more/better term definitions. Multiple AppContexts allowed?
//TODO: TouchContext. Other input sources?
///Abstract base interface for a backend-specific display conncetion.
///Defines the interfaces for different (threaded/blocking) event dispatching functions
///that have to be implemented by the different backends.
class AppContext : public NonCopyable
{
public:
    AppContext() = default;
    virtual ~AppContext() = default;

	///Creates a WindowContext implementation for the given settings.
	///May throw backend specific errors or nullptr if it fails.
	////\sa WindowContext
	virtual WindowContextPtr createWindowContext(const WindowSettings& windowSettings) = 0;

	///Returns a MouseContext implementation that is able to provide information about the mouse.
	///Note that this might return the same implementation for every call and the returned
	///object reference does not provide any kind of thread safety.
	///Returns nullptr if the implementation is not able to provide the needed
	///information about the pointer or there is no pointer connected.
	///Note that the fact that this function returns a valid pointer does not guarantee that
	///there is a mouse connected.
	///\sa MouseContext
	virtual MouseContext* mouseContext() = 0;

	///Returns a KeyboardContext implementation that is able to provide information about the
	///backends keyboard.
	///Note that this might return the same implementation for every call and the returned
	///object reference does not provide any kind of thread safety.
	///Returns nullptr if the implementatoin is not able to provide
	///the needed information about the keyboard or there is no keyboard connected.
	///Note that the fact that this function returns a valid pointer does not guarantee that
	///there is a keyboard connected.
	///\sa KeyboardContext
	virtual KeyboardContext* keyboardContext() = 0;

	///Dispatches all retrieved events to the registered handlers using the given EventDispatcher.
	///Does only dispatch all currently queued events and does not wait/block for new events.
	///This function is not threadsafe, i.e. it is undefined what happens it other threads
	///use the given EventDispatcher until the function returns.
	///\return false if the display conncetion was destroyed (or an error occurred),
	///true otherwise (if all queued events were dispatched).
	virtual bool dispatchEvents(EventDispatcher&) = 0;

	///Blocks and dispatches all incoming display events until the stop function of the loop control
	///is called or the display conncection is closed by the server (e.g. an error or exit event).
	///\param control A AppContext::LoopControl object that can be used to stop the loop from
	///inside or from another thread.
	///\return true if loop was exited because stop() was called by the LoopControl.
	///\return false if loop was exited because the display conncetion was destroyed.
	virtual bool dispatchLoop(EventDispatcher&, LoopControl&) = 0;

	///Blocks ans dispatches all incoming events from the display and queued events inside
	///the EventDispatcher until the stop function of the given loop control is called or
	///the display connection is closed by the server.
	///This function only accepts an ThreadedEventDispatcher as dispatcher paramter and
	///guarantess furthermore threadsafety, i.e. events from other threads dispatched over the
	///given EventDispatcher will be sent.
	///This function itself will take care of dispatching all events, so the dispatcher
	///loop of the given dispatcher should not be run in different threads.
	///\sa ThreadedEventDispatcher
	///\param control A AppContext::LoopControl object that can be used to stop the loop from
	///inside or from another thread.
	///\return true if loop was exited because stop() was called by the LoopControl.
	///\return false if loop was exited because the display conncetion was destroyed.
	virtual bool threadedDispatchLoop(ThreadedEventDispatcher&, LoopControl&) = 0;


	///Sets the clipboard to the data provided by the given DataSource implementation.
	///\param dataSource a DataSource implementation for the data to copy.
	///The data may be directly copied from the DataSource and the given object be destroyed,
	///or it may be stored by an AppContext implementation and retrieve the data only when
	///other applications need them.
	///Therefore the given DataSource implementation must be able to provide data as long
	///as it exists.
	///\return true on success, false on failure.
	///\sa DataSource
	virtual bool clipboard(std::unique_ptr<DataSource>&& dataSource) = 0;

	///Retrieves the data stored in the systems clipboard, or an nullptr if there is none.
	///The DataOffer implementation can then be used to get the data in the needed formats.
	///If the clipboard is empty or cannot be retrieved, returns nullptr.
	///\sa DataOffer
	virtual std::unique_ptr<DataOffer> clipboard() = 0;

	///Start a drag and drop action at the current cursor position.
	///Note that this function might not return (running an internal event loop)
	///until the drag and drop ends.
	///\param dataSource A DataSource implementation for the data to drag and drop.
	///The implementation must be able to provide the data as long as it exists.
	///\return true on success, false on failure (e.g. cursor is not over a window)
	virtual bool startDragDrop(std::unique_ptr<DataSource>&& dataSource) = 0;
};

}
