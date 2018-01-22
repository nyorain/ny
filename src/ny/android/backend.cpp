// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/backend.hpp>
#include <ny/android/activity.hpp>
#include <ny/android/appContext.hpp>
#include <nytl/tmpUtil.hpp>

#include <android/native_activity.h>
#include <thread>
#include <string>

namespace ny {

void AndroidBackend::initialize()
{
	static AndroidBackend instance_;
}

bool AndroidBackend::available() const
{
	return android::Activity::instance();
}

AppContextPtr AndroidBackend::createAppContext()
{
	auto instance = android::Activity::instance();
	if(!instance) {
		throw std::logic_error("ny::AndroidBackend::createAppContext: no native activity");
	}

	return std::make_unique<AndroidAppContext>(*instance);
}

} // namespace ny
