#include <ny/ny.hpp>
#include <ny/common/egl.hpp>

#include <windows.h>

constexpr unsigned int GL_COLOR_BUFFER_BIT = 0x00004000;
using PfnClearColor = void(*)(float, float, float, float);
using PfnClear = void(*)(unsigned int);

class MyWindowListener : public ny::WindowListener {
public:
	ny::AppContext* ac;
	ny::WindowContext* wc;
	ny::LoopControl* lc;

	ny::GlSetup* setup {};
	ny::GlContext* ctx {};
	ny::GlSurface* surface {};

	PfnClearColor gl_clearColor {};
	PfnClear gl_clear {};

public:
	void close(const ny::CloseEvent&) override
	{
		ny::log("Recevied closed event. Exiting");
		lc->stop();
	}

	void draw(const ny::DrawEvent&) override
	{
		if(!ctx || !ctx->isCurrent()) {
			ny::warning("not current!");
			return;
		}

		gl_clearColor(0.4, 0.6, 0.3, 0.8);
		gl_clear(GL_COLOR_BUFFER_BIT);
		surface->apply();
	}
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	MyWindowListener listener;

	ny::WindowSettings settings;
	settings.title = "Ayy sick shit";
	settings.listener = &listener;

	auto wc = ac->createWindowContext(settings);
	auto nativeWindow = wc->nativeHandle().pointer();
	auto hdc = ::GetDC((HWND) nativeWindow);

	ny::EglSetup eglSetup(hdc);

	auto conf = eglSetup.defaultConfig();
	auto eglConf = eglSetup.eglConfig(conf.id);

	ny::EglSurface surface(eglSetup.eglDisplay(), nativeWindow, conf, eglConf);

	ny::GlContextSettings ctxSettings;
	ctxSettings.version.api = ny::GlApi::gles;
	ny::EglContext context(eglSetup, ctxSettings);

	ny::LoopControl control;

	listener.lc = &control;
	listener.ac = ac.get();
	listener.wc = wc.get();

	listener.setup = &eglSetup;
	listener.surface = &surface;
	listener.ctx = &context;

	{
		ny::GlCurrentGuard currentGuard(context, surface);
		listener.gl_clearColor = (PfnClearColor) eglSetup.procAddr("glClearColor");
		listener.gl_clear = (PfnClear) eglSetup.procAddr("glClear");

		ny::log("Entering main loop");
		wc->refresh();
		ac->dispatchLoop(control);
	}
}
