//  This example is a basic example on how to use ny with opengl[es].

#include <ny/ny.hpp>
#include <ny/common/gl.hpp>
#include <dlg/dlg.hpp>

#ifdef NY_WithAndroid
	#include <GLES2/gl2.h>
#else
	#include <GL/gl.h>
#endif

class MyWindowListener : public ny::WindowListener {
public:
	ny::GlSurface* glSurface;
	std::unique_ptr<ny::GlContext> glContext;
	ny::WindowContext* windowContext;
	ny::AppContext* appContext;
	bool* run;

	void draw(const ny::DrawEvent&) override;
	void close(const ny::CloseEvent&) override;
	void surfaceCreated(const ny::SurfaceCreatedEvent&) override;
	void surfaceDestroyed(const ny::SurfaceDestroyedEvent&) override;
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	if(!backend.gl() || !ac->glSetup()) {
		dlg_error("The ny library was built without gl or failed to init it!");
		return EXIT_FAILURE;
	}

	// Here we specify that a gl surface should be created and then be stored in surface.
	// This context is guaranteed to be valid at least until the WindowContext it was created
	// for gets destructed.
	ny::GlSurface* glSurface {};

	ny::WindowSettings windowSettings;
	windowSettings.surface = ny::SurfaceType::gl;
	windowSettings.transparent = true;
	windowSettings.gl.storeSurface = &glSurface;

	auto wc = ac->createWindowContext(windowSettings);

	auto run = true;
	MyWindowListener listener;
	listener.glSurface = glSurface;
	listener.appContext = ac.get();
	listener.windowContext = wc.get();
	listener.run = &run;

	ny::GlContextSettings glsettings;
	if(glSurface) {
		listener.glContext = ac->glSetup()->createContext(*glSurface);
	}

	wc->listener(listener);

	dlg_info("Entering main loop");
	while(run) {
		ac->waitEvents();
	}
}

void MyWindowListener::close(const ny::CloseEvent&)
{
	dlg_info("Window closed from server side. Exiting.");
	*run = false;
	appContext->wakeupWait();
}

void MyWindowListener::draw(const ny::DrawEvent&)
{
	if(!glSurface) {
		dlg_warn("draw without gl surface");
	}

	// First, we make the gl context current.
	// we do not make it notCurrent and the end of this function since we only
	// deal with one context and it can stay current all the time.
	// This call does automatically detect if the context is already current and then
	// does not have to call the gl context api.
	glContext->makeCurrent(*glSurface);

	// note that you usually would have to set the viewport correctly
	// but since we only clear here, it does not matter.
	glClearColor(0.6, 0.5, 0.8, 0.5);
	glClear(GL_COLOR_BUFFER_BIT);

	// Finally, swap the buffers/apply the content
	glSurface->apply();
}

void MyWindowListener::surfaceCreated(const ny::SurfaceCreatedEvent& surfaceEvent)
{
	glSurface = surfaceEvent.surface.gl;

	if(!glContext)
		glContext = appContext->glSetup()->createContext(*glSurface);

	windowContext->refresh();
}

void MyWindowListener::surfaceDestroyed(const ny::SurfaceDestroyedEvent&)
{
	glSurface = nullptr;
}
