#pragma once

#include <ny/android/include.hpp>
#include <ny/windowContext.hpp>
#include <ny/windowSettings.hpp>

namespace ny
{

struct AndroidWindowSettings : public WindowSettings {};

///DrawIntegration base class for android WindowContexts.
class AndroidDrawIntegration
{
public:
	AndroidDrawIntegration(AndroidWindowContext&);
	~AndroidDrawIntegration();

protected:
	AndroidWindowContext& windowContext_;
};

///Android WindowContext implementation.
///Cannot implement many of the desktop-orientated functions correctly, i.e. has
///few capabilities.
///Wrapper around ANativeWindow functionality.
class AndroidWindowContext : public WindowContext
{
public:
	AndroidWindowContext(AndroidAppContext& ac, const AndroidWindowSettings& settings);
	~AndroidWindowContext();

	// - android specific -
	virtual bool surface(Surface&);
	virtual bool drawIntegration(AndroidDrawIntegration*);

	ANativeWindow& nativeWindow() const { return nativeWindow_; }

protected:
	ANativeWindow nativeWindow_;
};

}
