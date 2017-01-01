// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <cstdint> // std::intmax_t

namespace ny {

/// Class to reprsent some backend specific handle of e.g. a window.
/// Holds either a pointer to backend-specific type or an integer value.
class NativeHandle {
public:
	using Value = std::intmax_t;

public:
	NativeHandle() = default;
	template<typename T> NativeHandle(T* handle) : value_(reinterpret_cast<Value>(handle)) {}
	template<typename T> NativeHandle(T& handle) : value_(reinterpret_cast<Value>(&handle)) {}

	NativeHandle(std::uintmax_t uint) : value_(static_cast<Value>(uint)) {}
	NativeHandle(std::intmax_t sint) : value_(reinterpret_cast<Value>(sint)) {}

	void* pointer() const { return reinterpret_cast<void*>(value_); }
	Value value() const { return value_; }

	std::uint64_t uint64() const { return static_cast<std::uint64_t>(value_); }
	std::uintmax_t uintmax() const { return static_cast<std::uintmax_t>(value_); }
	std::uintptr_t uintptr() const { return static_cast<std::uintptr_t>(value_); }

	std::int64_t int64() const { return reinterpret_cast<std::int64_t>(value_); }
	std::intmax_t intmax() const { return reinterpret_cast<std::intmax_t>(value_); }
	std::intptr_t intptr() const { return reinterpret_cast<std::intptr_t>(value_); }

	template<typename T> T* asPtr() const { return reinterpret_cast<T*>(value_); }

	operator bool() const { return (value_); }

protected:
	Value value_ {};
};

} // namespace ny
