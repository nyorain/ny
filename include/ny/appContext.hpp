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
// TODO: TouchContext. Other input sources?

/// Abstract base interface for a backend-specific display conncetion.
/// Defines the interfaces for different event dispatching functions
/// that have to be implemented by the different backends.
/// Multiple AppContext's are allows but usually not useful.
class AppContext : public nytl::NonCopyable {
public:
	AppContext() = default;
	virtual ~AppContext() = default;

	/// Creates a WindowContext implementation for the given settings.
	/// May throw backend specific errors on failure, e.g. when the
	/// given WindowSettings are invalid or requirements can't be met (e.g.
	/// vulkan window requested but vulkan not supported).
	/// Note that this AppContext object must not be destroyed as long as the
	/// WindowContext exists.
	virtual WindowContextPtr createWindowContext(const WindowSettings&) = 0;

	/// Tries to read and dispatch all available events.
	/// Will not block waiting for events.
	/// Can be called from a handler from within the loop (recursively).
	/// Forwards all exceptions that are thrown in called handlers.
	/// Might also throw own backend-dependent errors.
	/// If these are of type ny::BackendError, the AppContext has become
	/// invalid and must no longer be used.
	/// This happens e.g. on protocol errors in communication with
	/// the display server.
	virtual void pollEvents() = 0;

	/// Waits until events are avilable to be read and dispatched.
	/// If there are immediately events available, behaves like pollEvents,
	/// otherwise waits until at least one event has ben processed.
	/// Can be called from a handler from within the loop (recursively).
	/// Use wakeupWait to return from this function.
	/// Forwards all exceptions that are thrown in called handlers.
	/// Might also throw own backend-dependent errors.
	/// If these are of type ny::BackendError, the AppContext has become
	/// invalid and must no longer be used.
	/// This happens e.g. on protocol errors in communication with
	/// the display server.
	virtual void waitEvents() = 0;

	/// Causes `waitEvents` to return even if no events could be dispatched.
	/// If `waitEvents` was called multiple times from within each other,
	/// will only make the most recent waitEvents call (i.e. the highest
	/// one on the call stack). Calling this without any currently
	/// active `waitEvents` call may cause the next call to `waitEvents`
	/// or `pollEvents` to return immediately (without processing an event)
	/// but must otherwise have no effect.
	/// Throws when an error ocurss, but the AppContext can still
	/// be used.
	virtual void wakeupWait() = 0;

	/// Returns a MouseContext implementation that is able to provide
	/// information about the mouse. The returned pointer may be null,
	/// is not threadsafe in any way and may become invalid as soon as control
	/// returns to wait/pollEvents (so don't store this value anywhere, just
	/// call this function again next time, it shouldn't have much overhead).
	/// This function returning a valid MouseContext does not guarantee
	/// that there actually is a mouse connected.
	virtual MouseContext* mouseContext() = 0;

	/// Returns a KeyboardContext implementation that is able to provide
	/// information about the keyboard. The returned pointer may be null,
	/// is not threadsafe in any way and may become invalid as soon as control
	/// returns to wait/pollEvents (so don't store this value anywhere, just
	/// call this function again next time, it shouldn't have much overhead).
	/// This function returning a valid Keyboard does not guarantee
	/// that there actually is a keyboard connected.
	virtual KeyboardContext* keyboardContext() = 0;

	/// Sets the clipboard to the given DataSource implementation.
	/// dataSource: A DataSource implementation providing the data
	/// that should be copied.
	/// The the given DataSource implementation must be able to provide data
	/// as long as it exists.
	/// Returns false if the backend has no clipboard support or setting
	/// the clipboard failed in another way.
	virtual bool clipboard(std::unique_ptr<DataSource>&& dataSource) = 0;

	/// Attempts to retrieve the data stored in the systems clipboard.
	/// The DataOffer implementation can then be used to get the data
	/// in the required formats. If the clipboard is empty or cannot be
	/// retrieved, or if the backend doesn't have a clipboard, returns nullptr.
	/// Note that the returned DataOffer (if any) is only guaranteed to be
	/// valid until control returns to poll/waitEvents. As soon as the
	/// DataOffer becomes invalid, it will notify the pending signals
	/// that their requests were unsuccessful.
	virtual DataOffer* clipboard() = 0;

	// TODO: might be problematic to let blocking depend on backend
	// but can't be done async on windows and making other backends block as
	// well seems bad

	/// Start a drag and drop action at the current cursor position.
	/// Note that this function might not return (running an internal event loop)
	/// until the drag and drop ends; depending on the backend.
	/// \param dataSource A DataSource implementation for the data to drag and drop.
	/// The implementation must be able to provide the data as long as it exists.
	/// \param event EventData of the event to which this action is the
	/// response. Should be a mouse event.
	/// \return Whether starting the dnd operation suceeded.
	virtual bool dragDrop(const EventData* trigger,
		std::unique_ptr<DataSource>&& dataSource) = 0;

	/// If ny was built with vulkan and the AppContext implementation has
	/// vulkan support, this returns all instance extensions that must be
	/// enabled for an instance to make it suited for vulkan surface creation
	/// and sets supported to true.
	/// If vulkan is not supported by backend/build returns an empty
	/// vector (can already be checked by Backend::vulkan).
	virtual std::vector<const char*> vulkanExtensions() const = 0;

	/// Returns a GlSetup implementation or nullptr if gl is not supported or
	/// initialization failed. The returned GlSetup can be used to retrieve
	/// the different gl configs and to create opengl contexts.
	virtual GlSetup* glSetup() const = 0;
};

} // namespace nytl
