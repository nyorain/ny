// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <nytl/nonCopyable.hpp> // nytl::NonMoavable

#include <vector> // std::vector
#include <memory> // std::unique_ptr

namespace ny {

using AppContextPtr = std::unique_ptr<AppContext>;
using WindowContextPtr = std::unique_ptr<WindowContext>;

/// Base class for backend implementations.
/// Can be used to retrieve a list of the built-in backends which can then be checked for
/// availability and used to create app or window contexts.
/// A Backend represents an abstract possibility to display something on screen and to
/// retrieve events from the system. Usually a backend represents a output method (display
/// manager protocol or direct) in combination with a method to retrieve input.
/// Example backend implementations are:
/// - Winapi for windows
/// - x11 for unix
/// - wayland for linux
/// But backends like android or linux drm + udev/libinput would be possible as well.
class Backend : public nytl::NonMovable {
public:
	/// Returns a list of all current registered backends.
	/// Note that backend implementations usually hold a static object of theiself, so this
	/// function will returned undefined contents if called before or after main().
	/// The Backends registered here are the backends that are loaded/ny was built with.
	/// It does not guarantee that they are available, this must be checked with Backend::available.
	static std::vector<Backend*> backends() { return backendsFunc(); }

	/// Chooses one available backend.
	/// Will prefer the backend set in the NY_BACKEND environment variable. If it is
	/// not available, will output a warning and choose another backend.
	/// \exception std::runtime_error if no backend is available.
	static Backend& choose();

public:
	/// Returns whether the backend is available.
	/// Here implementation can check whether they could e.g. connect to a server.
    virtual bool available() const = 0;

	/// Creates an AppContext that can be used to retrieve events from this backend or to create
	/// a WindowContext.
	/// \sa AppContext
    virtual AppContextPtr createAppContext() = 0;

	/// Returns the name of this backend.
	/// Example for backend names are e.g. "wayland", "x11" or "winapi".
	virtual const char* name() const = 0;

	/// Returns whether this backend does theoretically support gl.
	/// Will return false if it does not implement it or it was built without the
	/// needed libraries.
	/// Note that even if this returns true, gl initialization may still fail so that
	/// it does not guarantee that gl surfaces/contexts will work.
	virtual bool gl() const = 0;

	/// Returns whether this backend does theoretically support vulkan.
	/// Will return false if it does not implement it or it was built without the
	/// needed libraries.
	/// Note that even if this returns true, vulkan surface creation may still fail somehow, so
	/// dont treat this as a guarantee.
	virtual bool vulkan() const = 0;

protected:
	Backend() { backendsFunc(this); }
	~Backend() { backendsFunc(this, true); }

	// small helper func to add/remove backends from the backend vector singleton.
	static std::vector<Backend*> backendsFunc(Backend* reg = nullptr, bool remove = 0);
};

} // namespace ny
