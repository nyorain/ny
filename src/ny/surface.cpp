// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/surface.hpp>
#include <cstring>

namespace ny
{

//Cannot be defaulted to due some weird union stuff
Surface::Surface() : gl() {}
Surface::~Surface() {}

//TODO: std-conform implementation of this?
Surface::Surface(Surface&& other) noexcept
{
	std::memcpy(this, &other, sizeof(other));
	std::memset(&other, 0, sizeof(other));
}

Surface& Surface::operator=(Surface&& other) noexcept
{
	std::memcpy(this, &other, sizeof(other));
	std::memset(&other, 0, sizeof(other));
	return *this;
}


}
