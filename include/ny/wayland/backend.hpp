// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/backend.hpp>

namespace ny {

///Wayland backend implementation.
class WaylandBackend : public Backend {
public:
	/// Makes sure this backend is registerd
	static void initialize();

public:
	bool available() const override;
	AppContextPtr createAppContext() override;
	const char* name() const override { return "wayland"; }

	bool gl() const override;
	bool vulkan() const override;

protected:
	WaylandBackend() = default;
};

} // namespace ny
