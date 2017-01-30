//  This example is a basic example on how to use ny with opengl[es].

#include <ny/ny.hpp>
#include <ny/common/gl.hpp>
#include <GL/gl.h>

class MyWindowListener : public ny::WindowListener {
public:
	ny::GlSurface* glSurface;
	ny::GlContext* glContext;
	ny::LoopControl* loopControl;
	ny::WindowContext* windowContext;

	void draw(const ny::DrawEvent&) override;
	void close(const ny::CloseEvent&) override;
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

	ny::WindowSettings settings;
	settings.surface = ny::SurfaceType::gl;
	settings.transparent = false;
	settings.gl.storeSurface = &glSurface;

	auto wc = ac->createWindowContext(settings);

	// Create the opengl context
	// Important to pass the surfaces gl config here to make sure
	// we can use the context to render onto the surface
	auto glContext = ac->glSetup()->createContext({glSurface->config().id});
	glContext->makeCurrent(*glSurface);

	ny::LoopControl control;
	MyWindowListener listener;
	listener.glSurface = glSurface;
	listener.glContext = glContext.get();
	listener.windowContext = wc.get();
	listener.loopControl = &control;

	wc->listener(listener);
	wc->refresh();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}

void MyWindowListener::close(const ny::CloseEvent&)
{
	ny::log("Window closed from server side. Exiting.");
	loopControl->stop();
}

void MyWindowListener::draw(const ny::DrawEvent&)
{
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
