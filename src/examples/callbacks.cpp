#include <ny/base.hpp>
#include <ny/backend.hpp>

#include <nytl/vecOps.hpp>
#include <nytl/time.hpp>
#include <nytl/utf.hpp>

//This example shows how to handle certain events independent from any WindowContext.
//Note that this does not work without any WindowContext but it unites all created WindowContext
//events since receiving input without window does usually not work.

class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc) : lc_(mainLoop), wc_(wc) {}
	bool handleEvent(const ny::Event& ev) override;

protected:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	ny::WindowSettings settings;
	auto wc = ac->createWindowContext(settings);

	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	//XXX: interesting part here
	//First check that the AppContext does implement keyboard and mouse input.
	//This is no guarantee for actually receiving such input!
	auto kc = ac->keyboardContext(); //decltype(kc) = ny::KeyboardContext*
	auto mc = ac->mouseContext(); //decltype(mc) = ny::MouseCotnext*
	if(!kc || !mc)
	{
		ny::error("ny::AppContext does not have mouse and keyboard input.");
		return EXIT_FAILURE;
	}

	//We register callbacks for various keyboard and mouse events.
	//If you want to learn more about callbacks (really interesting, we could also
	//pass functions that dont take parameters we are not interested in), lookup
	//the nytl::Callback template class in the nytl repository.

	//This callback is called everytime a key is pressed or released.
	//We quit the window on escape (what we did previously using the EventHandler implementation).
	kc->onKey += [&](ny::Keycode key, const std::string& utf8, bool pressed) {
		ny::debug("Key ", ny::keycodeName(key), pressed ? " pressed: " : " released: ", utf8);
		if(key == ny::Keycode::escape) control.stop();
	};

	//This callback is called everytime our WindowContext gains or loses focus.
	kc->onFocus += [&](ny::WindowContext*, ny::WindowContext* now) {
		ny::debug("KeyboardFocus ", now ? "gained " : "lost ");
	};

	//This callback is called everytime the mouse moves
	//Becase this would literally spam the output, we only output the mouse position
	//if the cursor has travelled 500 pixels (absolute).
	nytl::Vec2ui absDeltaSum;
	nytl::TimePoint lastReset = nytl::Clock::now();
	mc->onMove += [&](nytl::Vec2i position, nytl::Vec2i delta) {
		absDeltaSum += nytl::abs(delta);
		if(length(absDeltaSum) > 500)
		{
			auto mis = nytl::duration_cast<nytl::Microseconds>(nytl::Clock::now() - lastReset);
			ny::debug("Mouse moved ", absDeltaSum, " in ", mis.count() / 1000, " milliseconds");
			ny::debug("\tCurrent mouse position in window: ", position);

			lastReset = nytl::Clock::now();
			absDeltaSum = {};
		}
	};

	//This callback is called everytime a mouse button is pressed or released.
	//We can also use the MouseContext implementation to query the current mouse position
	mc->onButton += [&](ny::MouseButton button, bool pressed) {
		ny::debug("Button ", ny::mouseButtonName(button), pressed ? " pressed " : " released");
		ny::debug("\tCurrent mouse position in window: ", mc->position());
	};

	//This callback is called everytime when the mouse leaves or enters our WindowContext.
	mc->onFocus += [&](ny::WindowContext*, ny::WindowContext* next) {
		ny::debug("Mouse now ", next ? " over " : " not over ", " WindowContext");
	};

	//This callback is called everytime the mouse wheel is rotated
	mc->onWheel += [&](float value) {
		ny::debug("MouseWheel rotated: ", value);
	};

	wc->eventHandler(handler);
	wc->refresh();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}

bool MyEventHandler::handleEvent(const ny::Event& ev)
{
	static std::unique_ptr<ny::DataOffer> offer;

	if(ev.type() == ny::eventType::close)
	{
		ny::debug("Window closed. Exiting.");
		lc_.stop();
		return true;
	}

	return false;
}
