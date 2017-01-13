#include <ny/ny.hpp>
#include <ny/common/gl.hpp>

//XXX: This example is a basic example on how to use ny with opengl[es].

//We load the 2 needed gl functions ourself because everything involving the opengl public
//interface is a mess.
//Note that ny does not load the gl functions, use your preferred dynamic gl loader for this.
constexpr unsigned int GL_COLOR_BUFFER_BIT = 0x00004000;
using PfnClearColor = void(*)(float, float, float, float);
using PfnClear = void(*)(unsigned int);
PfnClearColor gl_clearColor;
PfnClear gl_clear;

class MyEventHandler : public ny::WindowListener {
public:
	ny::GlSurface* surface;
	ny::GlContext* ctx;

public:
public:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	if(!backend.gl() || !ac->glSetup()) {
		ny::error("The ny library was built without gl or failed to init it!");
		return EXIT_FAILURE;
	}

	//Here we specify that a gl surface should be created and then be stored in surface.
	//This context is guaranteed to be valid at least until the WindowContext it was created
	//for gets destructed.
	ny::GlSurface* surface {};

	ny::WindowSettings settings;
	settings.surface = ny::SurfaceType::gl;
	settings.gl.storeSurface = &surface;

	auto wc = ac->createWindowContext(settings);

	//Create the opengl context
	//Here, we just use the default config and settings
	auto ctx = ac->glSetup()->createContext();
	ctx->makeCurrent(*surface);

	//load the needed gl functions manually
	gl_clearColor = reinterpret_cast<PfnClearColor>(ac->glSetup()->procAddr("glClearColor"));
	gl_clear = reinterpret_cast<PfnClear>(ac->glSetup()->procAddr("glClear"));

	//check that the functions could be loaded
	if(!gl_clearColor || !gl_clear)
	{
		ny::warning("Could not get the required gl functions");
		return EXIT_FAILURE;
	}

	///With this object we can stop the dispatchLoop called below from inside.
	///We construct the EventHandler with a reference to it and when it receives an event that
	///the WindowContext was closed, it will stop the dispatchLoop, which will end this
	///program.
	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	///This call registers our EventHandler to receive the WindowContext related events from
	///the dispatchLoop.
	wc->eventHandler(handler);
	wc->refresh();

	//pass the context to the event handler
	handler.ctx = ctx.get();
	handler.surface = surface;

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}

bool MyEventHandler::handleEvent(const ny::Event& ev)
{
	ny::debug("Received event with type ", ev.type());

	if(ev.type() == ny::eventType::close)
	{
		ny::debug("Window closed from server side. Exiting.");
		lc_.stop();
		return true;
	}
	else if(ev.type() == ny::eventType::draw)
	{
		//First, we make the gl context current.
		//we do not make it notCurrent and the end of this function since we only
		//deal with one context and it can stay current all the time.
		//This call does automatically detect if the context is already current and then
		//does not have to call the gl context api.
		ctx->makeCurrent(*surface);

		//note that you usually would have to set the viewport correctly
		//but since we only clear here, it does not matter.
		gl_clearColor(0.6, 0.5, 0.3, 0.5);
		gl_clear(GL_COLOR_BUFFER_BIT);

		//Finally, swap the buffers/apply the content
		surface->apply();

		return true;
	}
	else if(ev.type() == ny::eventType::key)
	{
		const auto& kev = static_cast<const ny::KeyEvent&>(ev);
		if(!kev.pressed) return false;

		if(kev.keycode == ny::Keycode::escape)
		{
			ny::debug("Esc key pressed. Exiting.");
			lc_.stop();
			return true;
		}
		else if(kev.keycode == ny::Keycode::f)
		{
			ny::debug("f key pressed. Fullscreen.");
			wc_.fullscreen();
			return true;
		}
		else if(kev.keycode == ny::Keycode::n)
		{
			ny::debug("n key pressed. Normal.");
			wc_.normalState();
			return true;
		}
		else if(kev.keycode == ny::Keycode::i)
		{
			ny::debug("i key pressed. Iconic (minimize).");
			wc_.minimize();
			return true;
		}
		else if(kev.keycode == ny::Keycode::m)
		{
			ny::debug("m key pressed. maximize.");
			wc_.maximize();
			return true;
		}
	}

	return false;
}
