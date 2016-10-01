#include <ny/base.hpp>
#include <ny/backend.hpp>

//XXX: This example is a basic example on how to use ny with opengl[es].

//We load the 2 needed gl functions ourself because everything involving the opengl public
//interface is a mess.
//Note that ny does not load the gl functions, use your preferred dynamic gl loader for this.
constexpr unsigned int GL_COLOR_BUFFER_BIT = 0x00004000;
using PfnClearColor = void(*)(float, float, float, float);
using PfnClear = void(*)(unsigned int);
PfnClearColor gl_clearColor;
PfnClear gl_clear;


class MyEventHandler : public ny::EventHandler
{
public:
	ny::GlContext* ctx;

public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc) : lc_(mainLoop), wc_(wc) {}
	bool handleEvent(const ny::Event& ev) override;

public:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	//Here we specify that a gl context should be created and then be stored in ctx.
	//This context is guaranteed to be valid at least until the WindowContext it was created
	//for gets destructed.
	//We also disable vsync.
	ny::GlContext* ctx {};

	ny::WindowSettings settings;
	settings.context = ny::ContextType::gl;
	settings.gl.storeContext = &ctx;
	settings.gl.vsync = false;
	auto wc = ac->createWindowContext(settings);

	//check that the glcontext could be created
	if(!ctx)
	{
		ny::error("Could not create/retrieve the ny gl context.");
		return EXIT_FAILURE;
	}

	//load the needed gl functions manually
	gl_clearColor = reinterpret_cast<PfnClearColor>(ctx->procAddr("glClearColor"));
	gl_clear = reinterpret_cast<PfnClear>(ctx->procAddr("glClear"));

	//check that the functions could be loaded
	if(!gl_clearColor || !gl_clear)
	{
		ny::warning("Could not get the required gl functions");
		return EXIT_FAILURE;
	}

	//output some debug information
	ny::debug("Gl version: ", ny::name(ctx->version()));
	ny::debug("Preferred glsl version: ", ny::name(ctx->preferredGlslVersion()));
	ny::debug("Gl Extensions:");
	for(const auto& e : ctx->glExtensions())
		ny::debug("\t", e);

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
	handler.ctx = ctx;

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
		ctx->makeCurrent();

		//note that you usually would have to set the viewport correctly
		//but since we only clear here, it does not matter.
		gl_clearColor(0.6, 0.3, 0.3, 1.0);
		gl_clear(GL_COLOR_BUFFER_BIT);

		//Finally, swap the buffers/apply the content
		ctx->apply();

		return true;
	}
	else if(ev.type() == ny::eventType::key)
	{
		if(!static_cast<const ny::KeyEvent&>(ev).pressed) return false;

		ny::debug("Key pressed. Exiting.");
		lc_.stop();
		return true;
	}

	return false;
};
