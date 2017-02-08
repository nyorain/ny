// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/backend.hpp>

namespace ny {

/// Android backend implementation.
class AndroidBackend : public Backend {
public:
	static AndroidBackend& instance() { return instance_; }

protected:
	bool available() const override;
	AppContextPtr createAppContext() override;
	const char* name() const override { return "android"; }

	bool gl() const override { return builtWithGl(); }
	bool vulkan() const override { return builtWithVulkan(); }

protected:
	static AndroidBackend instance_;
	AndroidBackend() = default;
};

} // namespace ny
