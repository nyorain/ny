// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <type_traits>
#include <cstring>
#include <cmath>

namespace ny {

/// Casts an event from one type into an event of another type
/// by using memcpy.
template<typename To, typename From>
auto copy(const From& val) {
	To to;
	std::memcpy(&to, &val, std::min(sizeof(to), sizeof(From)));
	return to;
}

/// Like copy_cast but only works if both types have the same size
template<typename To, typename From,
	typename = std::enable_if_t<sizeof(To) == sizeof(From)>>
auto copyf(const From& val) {
	To to;
	std::memcpy(&to, &val, sizeof(to));
	return to;
}

} // namespace ny
