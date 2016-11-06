#pragma once

#include <ny/android/include.hpp>
#include <ny/appContext.hpp>

#include <android/native_activity.h>

namespace ny
{

///Android AppContext implementation.
class AndroidAppContext : public AppContext
{
public:
	AndroidAppContext();
	~AndroidAppContext();

	WindowContextPtr createWindowContext(const WindowSettings& settings) override;
	MouseContext* mouseContext() override { return nullptr; }
	KeyboardContext* keyboardContext() override;

	bool dispatchEvents() override;
	bool dispatchLoop(LoopControl&) override;
	bool threadedDispatchLoop(EventDispatcher&, LoopControl&) override;

	bool clipboard(std::unique_ptr<DataSource>&& dataSource) override;
	DataOffer* clipboard() override;

	std::vector<const char*> vulkanExtensions() const override;
	GlSetup* glSetup() const override;

	// - android specific -
	EglSetup* eglSetup() const;
	ANativeActivity& androidNaitveActivity() const { return *nativeActivtiy_; }

protected:
	ANativeActivity* nativeActivity_ {};

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

}
