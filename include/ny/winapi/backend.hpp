// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/winapi/include.hpp>
#include <ny/backend.hpp>

namespace ny {

/// Winapi backend implementation
class WinapiBackend : public Backend {
public:
	/// Makes sure this backend is registerd
	static void initialize();

public:
	bool available() const override { return true; } // TODO: sth to check here?
	AppContextPtr createAppContext() override;
	const char* name() const override { return "winapi"; }

	bool gl() const override;
	bool vulkan() const override;

protected:
	WinapiBackend() = default;
};

} // namespace ny
