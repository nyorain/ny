#include <ny/base.hpp>
#include <ny/backend.hpp>

//We load the 2 needed gl functions ourself because everything involving the opengl public
//interface is a mess.
using PfnClearColor = void(*)(float, float, float, float);
using PfnClear = void(*)(unsigned int);
constexpr unsigned int GL_COLOR_BUFFER_BIT = 0x00004000;

PfnClearColor gl_clearColor;
PfnClear gl_clear;

///Custom event handler for the low-level backend api.
///See intro-app for a higher level example if you think this is too complex.
class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc)
		: loopControl_(mainLoop), wc_(wc) {}

	///Virtual function from ny::EventHandler
	bool handleEvent(const ny::Event& ev) override
	{
		ny::debug("Received event with type ", ev.type());

		if(ev.type() == ny::eventType::close)
		{
			ny::debug("Window closed from server side. Exiting.");
			loopControl_.stop();
			return true;
		}
		else if(ev.type() == ny::eventType::draw)
		{
			ny::debug("Drawing...");

			//note that you usually would have to set the viewport correctly
			//but since we only clear here, it does not matter.
			//makes the underlaying gl context current
			ctx_->makeCurrent();
			gl_clearColor(0.6, 0.3, 0.3, 1.0);
			gl_clear(GL_COLOR_BUFFER_BIT);

			//swaps the buffers/applies the contents
			ctx_->apply();

			return true;
		}
		else if(ev.type() == ny::eventType::key)
		{
			if(!static_cast<const ny::KeyEvent&>(ev).pressed) return false;

			ny::debug("Key pressed. Exiting.");
			loopControl_.stop();
			return true;
		}
		else if(ev.type() == ny::eventType::size)
		{
			size_ = static_cast<const ny::SizeEvent&>(ev).size;
			return true;
		}

		return false;
	};

public:
	nytl::Vec2ui size_;
	ny::GlContext* ctx_;
	ny::LoopControl& loopControl_;
	ny::WindowContext& wc_;
};


///Main function that just chooses a backend, creates Window- and AppContext from it, registers
///a custom EventHandler and then runs the mainLoop.
int main()
{
	///We let ny choose a backend.
	///If no backend is available, this function will simply throw.
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	///Here we specify that a gl context should be created and then be stored in ctx.
	///This context is guaranteed to be valid at least until the WindowContext it was created
	///for gets destructed.
	ny::GlContext* ctx;

	ny::WindowSettings settings;
	settings.context = ny::ContextType::gl;
	settings.gl.storeContext = &ctx;
	settings.gl.vsync = false;
	auto wc = ac->createWindowContext(settings);

	//check that the glcontext could be created
	if(!ctx)
	{
		ny::warning("Could not create/retrieve the ny gl context.");
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
	handler.ctx_ = ctx;

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}
