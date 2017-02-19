// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/backend.hpp>

namespace ny {

/// X11 backend implementation.
class X11Backend : public Backend {
public:
	static X11Backend& instance(){ return instance_; }

public:
	bool available() const override;
	AppContextPtr createAppContext() override;
	const char* name() const override { return "x11"; }

	bool gl() const override;
	bool vulkan() const override;

protected:
	static X11Backend instance_;
	X11Backend();
};

} // namespace ny
