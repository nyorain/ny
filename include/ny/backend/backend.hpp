#pragma once

#include <ny/include.hpp>
#include <nytl/nonCopyable.hpp>

#include <vector>
#include <memory>
#include <string>

namespace ny
{

//Convinient typedefs.
using AppContextPtr = std::unique_ptr<AppContext>;
using WindowContextPtr = std::unique_ptr<WindowContext>;

///Base class for backend implementations.
///Can be used to retrieve a list of the built-in backends which can then be checked for
///availability and used to create app or window contexts.
///A Backend represents an abstract possibility to display something on screen and to 
///retrieve events from the system. Usually a backend represents a output method (display
///manager protocol or direct) in combination with a method to retrieve input.
///Example backend implementations are:
///- Winapi for windows
///- x11 for unix
///- wayland for linux
///But backends like linux drm + udev/libinput would be possible as well.
class Backend : public nytl::NonMovable
{
public:
	///Returns a list of all current registered backends.
	///Note that backend implementations usually hold a static object of theiself, so this
	///function will returned undefined contents if called before or after main().
	static std::vector<Backend*> backends() { return backendsFunc(); }

public:
	///Returns whether the backend is available.
    virtual bool available() const = 0;

	///Creates an AppContext that can be used to retrieve events from this backend or to create
	///a WindowContext.
	///\sa AppContext
    virtual AppContextPtr createAppContext() = 0;

	///Returns the name of this backend.
	///Example for backend names are e.g. "wayland", "x11" or "winapi".
	virtual const char* name() const = 0;

protected:
	Backend() { backendsFunc(this); }
	~Backend() { backendsFunc(this, true); }

	//small helper func to add/remove backends from the static variable.
	static std::vector<Backend*> backendsFunc(Backend* reg = nullptr, bool remove = 0);
};

}
