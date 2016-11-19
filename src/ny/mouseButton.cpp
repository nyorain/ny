// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/mouseButton.hpp>

namespace ny
{

constexpr struct Mapping
{
	MouseButton button;
	const char* name;
} mappings[] =
{
	{MouseButton::none, "none"},
	{MouseButton::unknown, "unknown"},

	{MouseButton::left, "left"},
	{MouseButton::right, "right"},
	{MouseButton::middle, "middle"},
	{MouseButton::custom1, "custom1"},
	{MouseButton::custom2, "custom2"},
	{MouseButton::custom3, "custom3"},
	{MouseButton::custom4, "custom4"},
	{MouseButton::custom5, "custom5"},
	{MouseButton::custom6, "custom6"},

	{MouseButton::left, "l"},
	{MouseButton::right, "r"},
	{MouseButton::middle, "m"},
	{MouseButton::custom1, "x1"},
	{MouseButton::custom2, "x2"},
};

const char* mouseButtonName(MouseButton button)
{
	for(auto m : mappings) if(m.button == button) return m.name;
	return "";
}

MouseButton mouseButtonFromName(nytl::StringParam name)
{
	for(auto m : mappings) if(m.name == name) return m.button;
	return MouseButton::none;
}

}
