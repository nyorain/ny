#pragma once

#include <ny/android/include.hpp>
#include <ny/backend.hpp>

namespace ny
{

///Android backend implementation.
class AndroidBackend : public Backend
{
public:
	static AndroidBackend& instance() { return instance_; }

protected:
	bool available() const override { return true; } //TODO: sth to check here?
	AppContextPtr createAppContext() override;
	const char* name() const override { return "android"; }

protected:
	static AndroidBackend instance_;
	AndroidBackend() = default;
};

}

#ifndef NY_WithAndroid
	#error ny was built without android, don't include this header file.
#endif //Android
