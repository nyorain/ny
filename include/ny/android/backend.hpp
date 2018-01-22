// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/backend.hpp>

namespace ny {

/// Android backend implementation.
class AndroidBackend : public Backend {
public:
	/// Makes sure this backend is registerd
	static void initialize();

protected:
	bool available() const override;
	AppContextPtr createAppContext() override;
	const char* name() const override { return "android"; }

	bool gl() const override { return builtWithEgl(); }
	bool vulkan() const override { return builtWithVulkan(); }

protected:
	AndroidBackend() = default;
};

} // namespace ny
