#pragma once

#include <ny/android/include.hpp>
#include <ny/android/windowContext.hpp>

namespace ny
{

class AndroidEglWindowContext : public AndroidWindowContext
{
public:
	AndroidEglWindowContext(AndroidAppContext&, EglSetup&, const AndroidWindowSettings&);
	~AndroidEglWindowContext() = default;

	bool drawIntegration(AndroidDrawIntegration*) override { return false; }
	bool surface(Surface&) override;

	EglSurface& eglSurface() const { return *surface_; }

protected:
	std::unique_ptr<EglSurface> surface_;
};

}
