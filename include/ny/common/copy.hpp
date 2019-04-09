// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <type_traits>
#include <cstring>
#include <cmath>

namespace ny {

/// Bytewise casts of an object from one type into an object of another type
/// by using memcpy. Only works if both types have the same size.
/// Somewhat comparable to C++20s bit_cast (but not as strict here).
template<typename To, typename From,
	typename = std::enable_if_t<sizeof(To) == sizeof(From)>>
auto copy(const From& val) {
	To to;
	std::memcpy(&to, &val, sizeof(to));
	return to;
}

/// Unsafe variant of copy.
/// Performs no size checks and instead *fully* fills the copied object.
/// This means that the value/buffer referenced by val should be at
/// least sizeof(To) bytes long.
template<typename To, typename From>
auto copyu(const From& val) {
	To to;
	std::memcpy(&to, &val, sizeof(To));
	return to;
}

} // namespace ny
