//  This example is a basic example on how to use ny with opengl[es].

#include <ny/ny.hpp>
#include <ny/common/gl.hpp>
#include <GLES2/gl2.h>

class MyWindowListener : public ny::WindowListener {
public:
	ny::GlSurface* glSurface;
	std::unique_ptr<ny::GlContext> glContext;
	ny::LoopControl* loopControl;
	ny::WindowContext* windowContext;
	ny::AppContext* appContext;

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
		ny::error("The ny library was built without gl or failed to init it!");
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

	ny::LoopControl control;
	MyWindowListener listener;
	listener.glSurface = glSurface;
	listener.appContext = ac.get();
	listener.windowContext = wc.get();
	listener.loopControl = &control;

	ny::GlContextSettings glsettings;
	if(glSurface)
		listener.glContext = ac->glSetup()->createContext(*glSurface);

	wc->listener(listener);
	wc->refresh();

	ny::log("Entering main loop");
	ac->dispatchLoop(control);
}

void MyWindowListener::close(const ny::CloseEvent&)
{
	ny::log("Window closed from server side. Exiting.");
	loopControl->stop();
}

void MyWindowListener::draw(const ny::DrawEvent&)
{
	if(!glSurface)
		ny::log("draw without gl surface");

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
	ny::log("surfaceCreated");
	glSurface = surfaceEvent.surface.gl;

	if(!glContext)
		glContext = appContext->glSetup()->createContext(*glSurface);

	windowContext->refresh();
}

void MyWindowListener::surfaceDestroyed(const ny::SurfaceDestroyedEvent&)
{
	ny::log("surfaceDestroyed");
	glSurface = nullptr;
}
