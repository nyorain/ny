// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <nytl/nonCopyable.hpp> // nytl::NonCopyable

#include <memory> // std::unique_ptr
#include <vector> // std::vector

namespace ny {

using WindowContextPtr = std::unique_ptr<WindowContext>;
using AppContextPtr = std::unique_ptr<AppContext>;

//TODO: optional (mouse)event parameter for dnd/clipboard functions?
//TODO: more/better term definitions. Multiple AppContexts allowed?
//TODO: return type of startDragDrop? AsyncRequest (docs/dev.md)
//TODO: TouchContext. Other input sources?

/// Abstract base interface for a backend-specific display conncetion.
/// Defines the interfaces for different (threaded/blocking) event dispatching functions
/// that have to be implemented by the different backends.
class AppContext : public nytl::NonCopyable {
public:
	AppContext() = default;
	virtual ~AppContext() = default;

	/// Creates a WindowContext implementation for the given settings.
	/// May throw backend specific errors on failure.
	/// Note that this AppContext object must not be destroyed as long as the WindowContext
	/// exists.
	/// /\sa WindowContext
	virtual WindowContextPtr createWindowContext(const WindowSettings&) = 0;

	/// Returns a MouseContext implementation that is able to provide information about the mouse.
	/// Note that this might return the same implementation for every call and the returned
	/// object reference does not provide any kind of thread safety.
	/// Returns nullptr if the implementation is not able to provide the needed
	/// information about the pointer or there is no pointer connected.
	/// Note that the fact that this function returns a valid pointer does not guarantee that
	/// there is a mouse connected.
	/// The lifetime of the object the returned pointer points to is allowed to end IN a dispatch
	/// call. So the returned pointer is guaranteed to be valid as long as no dispatch function is
	/// called.
	/// At the moment ny has no support for multiple mouse devices.
	/// \sa MouseContext
	virtual MouseContext* mouseContext() = 0;

	/// Returns a KeyboardContext implementation that is able to provide information about the
	/// backends keyboard.
	/// Note that this might return the same implementation for every call and the returned
	/// object reference does not provide any kind of thread safety.
	/// Returns nullptr if the implementatoin is not able to provide
	/// the needed information about the keyboard or there is no keyboard connected.
	/// Note that the fact that this function returns a valid pointer does not guarantee that
	/// there is a keyboard connected.
	/// The lifetime of the object the returned pointer points to is allowed to end IN a dispatch
	/// call. So the returned pointer is guaranteed to be valid as long as no dispatch function is
	/// called.
	/// At the moment ny has no support for multiple keyboard devices.
	/// \sa KeyboardContext
	virtual KeyboardContext* keyboardContext() = 0;

	/// Dispatches all retrieved events to their eventHandlers.
	/// Does only dispatch all currently queued events and does not wait/block for new events.
	/// Should only be called from the ui thread.
	/// \return false if the display conncetion was destroyed (or an error occurred) and the
	/// AppContext should no longer be used, true otherwise (if all queued events were dispatched).
	virtual bool dispatchEvents() = 0;

	/// Blocks and dispatches all incoming display events until the stop function of the loop control
	/// is called or the display conncection is closed by the server (e.g. an error or exit event).
	/// Shall only be called from the ui thread.
	/// \param control A LoopControl object that can be used to control the loop from
	/// inside or from another thread.
	/// \return true if loop was exited because stop() was called by the LoopControl.
	/// \return false if loop was exited because the display conncetion was destroyed or an error
	/// occured. The AppContext should then no longer be used.
	virtual bool dispatchLoop(LoopControl& control) = 0;

	/// Sets the clipboard to the data provided by the given DataSource implementation.
	/// \param dataSource a DataSource implementation for the data to copy.
	/// The data may be directly copied from the DataSource and the given object be destroyed,
	/// or it may be stored by an AppContext implementation and retrieve the data only when
	/// other applications need them.
	/// Therefore the given DataSource implementation must be able to provide data as long
	/// as it exists.
	/// \return true on success, false on failure.
	/// \sa DataSource
	virtual bool clipboard(std::unique_ptr<DataSource>&& dataSource) = 0;

	/// Retrieves the data stored in the systems clipboard, or an nullptr if there is none.
	/// The DataOffer implementation can then be used to get the data in the needed formats.
	/// If the clipboard is empty or cannot be retrieved, returns nullptr.
	/// Some backends also have no clipboard support at all, then nullptr should be returned
	/// as well.
	/// Note that the returned DataOffer (if any) is only guaranteed to be valid until the next
	/// dispatch call. If the clipboard content changes or is unset while there are still
	/// callbacks waiting for data in the requestes formats, they should be called
	/// without any data to signal that data retrieval failed. Then one should simply
	/// try again to retrieve the data since the clipboard changed before the data
	/// could be transmitted.
	/// \sa DataOffer
	virtual DataOffer* clipboard() = 0;

	/// Start a drag and drop action at the current cursor position.
	/// Note that this function might not return (running an internal event loop)
	/// until the drag and drop ends. Anyways may no further mouse events sent until
	/// the drag and drop ends.
	/// \param dataSource A DataSource implementation for the data to drag and drop.
	/// The implementation must be able to provide the data as long as it exists.
	/// \return true on success, false on failure (e.g. cursor is not over a window)
	virtual bool startDragDrop(std::unique_ptr<DataSource>&& dataSource) = 0;

	/// If ny was built with vulkan and the AppContext implementation has
	/// vulkan support, this returns all instance extensions that must be enabled for
	/// an instance to make it suited for vulkan surface creation and sets supported to true.
	/// Otherwise this returns an empty vector and sets supported to false.
	virtual std::vector<const char*> vulkanExtensions() const = 0;

	/// Returns a non-owned GlSetup implementation or nullptr if gl is not supported or
	/// initialization failed.
	/// The returned GlSetup can be used to retrieve the different gl configs and to create
	/// opengl contexts.
	virtual GlSetup* glSetup() const = 0;
};

} // namespace nytl
