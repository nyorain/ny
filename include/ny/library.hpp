// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <nytl/stringParam.hpp> // nytl::StringParam

namespace ny {

// The constructor of Library does not throw an excpetion since Library allows an invalid
// state (since it is movable) and loading libraries might be done using trial and error.
// It is pretty usual that a library is not found or cannot be loaded.
// The invalid state of the library does furthermore not cause any problems since the only
// operation it has (symbol) does simply return a nullptr if the Library object is invalid.
//
// The implementation is compile time defined. If ny is built on windows it will use
// the winapi functionality, otherwise dlfcn.h.

/// Cross-platform loaded dynamic library.
/// Basically abstracts unix dlfcn and winapi.
class Library {
public:
	Library() = default;

	/// Tries to load the library with the given name.
	/// On failure the library handle is invalid (i.e. handle() == nullptr).
	Library(nytl::StringParam name);
	~Library();

	Library(Library&& other) noexcept;
	Library& operator=(Library&& other) noexcept;

	/// Returns the associated native handle.
	/// If the Library is invalid, nullptr is returned.
	void* handle() const { return handle_; }

	/// Tries to find the symbol for the given name.
	/// Returns nullptr if the symbol was not found or the library handle
	/// is invalid.
	void* symbol(nytl::StringParam name) const;

	/// Converts the Library object to a bool that specifies whether the object
	/// refers to a valid handle.
	operator bool() const { return (handle()); }

protected:
	void* handle_ {};
};

} // namespace ny
