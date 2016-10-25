#pragma once

#include <ny/fwd.hpp>
#include <nytl/stringParam.hpp>

#include <system_error>

namespace ny
{

//The constructor of Library does not throw an excpetion since Library allows an invalid
//state (since it is movable) and loading libraries might be done using trial and error.
//It is pretty common that a library is not found or cannot be loaded.
//The invalid state of the library does furthermore not cause any problems since the only
//operation it has (symbol) does simply return a nullptr if the Library object is invalid.

//The implementation is compile time defined. If ny is built on windows it will use
//the winapi functionality, otherwise dlfcn.h.

//TODO: open flags
///Cross-platform loaded dynamic library.
class Library
{
public:
	Library() = default;

	///Tries to load the library with the given name.
	///On failure the library handle is invalid (i.e. handle() == nullptr).
	///\param error Can be used to receive the error if the constructor fails.
	Library(nytl::StringParam name, std::error_code* error = nullptr);
	~Library();

	Library(Library&& other) noexcept;
	Library& operator=(Library&& other) noexcept;

	///Returns the associated native handle.
	///If the Library is invalid, nullptr is returned.
	void* handle() const { return handle_; }

	///Tries to find the symbol for the given name.
	///Returns nullptr if the symbol was not found or the library handle
	///is invalid.
	void* symbol(nytl::StringParam name) const;

	///Converts the Library object to a bool that specifies whether the object
	///refers to a valid handle.
	operator bool() const { return (handle()); }

protected:
	void* handle_ {};
};

}
