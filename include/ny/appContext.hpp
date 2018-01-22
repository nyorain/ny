// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <nytl/nonCopyable.hpp> // nytl::NonCopyable

#include <memory> // std::unique_ptr
#include <vector> // std::vector
#include <functional> // std::function

namespace ny {

using WindowContextPtr = std::unique_ptr<WindowContext>;
using AppContextPtr = std::unique_ptr<AppContext>;

// TODO: optional (mouse)event parameter for dnd/clipboard functions?
// TODO: more/better term definitions. Multiple AppContexts allowed?
// TODO: return type of startDragDrop? AsyncRequest (docs/dev.md)
// TODO: TouchContext. Other input sources?
// TODO: wakeupWait: return how many layered wait calls are left?
// TODO: error handling (wait/poll/wakeup calls)

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
	virtual KeyboardContext* keyboardContext() = 0;

	/// Tries to read and dispatch all available events.
	/// Can be called from a handler from within the loop (recursively).
	/// Will not block waiting for events.
	/// Forwards all exceptions.
	/// If this function returns false, the AppContext should no longer
	/// be used. Don't automatically assume that this is an error, it might
	/// also occur when the display server shut down or the application
	/// is supposed to exit.
	virtual bool pollEvents() = 0;

	/// Waits until events are avilable to be read and dispatched.
	/// If there are e.g. immediately events available, behaves like pollEvents.
	/// Can be called from a handler from within the loop (recursively).
	/// Use wakeupWait to return from this function.
	/// If this function returns false, the AppContext should no longer
	/// be used. Don't automatically assume that this is an error, it might
	/// also occur when the display server shut down or the application
	/// is supposed to exit.
	virtual bool waitEvents() = 0;

	/// Causes waitEvents to return even if no events could be dispatched.
	/// If waitEvents was called multiple times from within each other,
	/// will only make the top most wait call return.
	/// Throws when an error ocurss, but the AppContext can still
	/// be used.
	virtual void wakeupWait() = 0;

	/// Sets the clipboard to the data provided by the given DataSource implementation.
	/// \param dataSource a DataSource implementation for the data to copy.
	/// The data may be directly copied from the DataSource and the given object be destroyed,
	/// or it may be stored by an AppContext implementation and retrieve the data only when
	/// other applications need them.
	/// Therefore the given DataSource implementation must be able to provide data as long
	/// as it exists.
	/// \return Whether settings the clipboard succeeded.
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
	/// until the drag and drop ends.
	/// \param dataSource A DataSource implementation for the data to drag and drop.
	/// The implementation must be able to provide the data as long as it exists.
	/// \return Whether starting the dnd operation suceeded.
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
