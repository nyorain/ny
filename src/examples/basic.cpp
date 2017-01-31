// This time, we explicitly include only the needed headers
#include <ny/backend.hpp> // ny::Backend
#include <ny/appContext.hpp> // ny::AppContext
#include <ny/windowContext.hpp> // ny::WindowContext
#include <ny/windowListener.hpp> // ny::WindowListener
#include <ny/windowSettings.hpp> // ny::WindowSettings
#include <ny/loopControl.hpp> // ny::LoopControl
#include <ny/bufferSurface.hpp> // ny::BufferSurface
#include <ny/log.hpp> // ny::log
#include <ny/key.hpp> // ny::Keycode
#include <ny/mouseButton.hpp> // ny::MouseButton
#include <ny/image.hpp> // ny::Image
#include <ny/event.hpp> // ny::*Event

#include <cstring> // std::memset

// The second ny example that shows some further basic functionality
// The example window has the following shortcuts (keycodes):
// 	- d: try to toggle server decorations
//	- f: toggle fullscreen
//	- m: toggle maximized state
// 	- i: iconize (minimize) the window
//	- n: reset to normal toplevel state
//	- Escape: close the window
// It uses a raw buffer to fill the window with a solid white color.
// Clicking on a the windows edges will start to resize it, while clicking in the middle
// of the window will start to move it.

// The WindowListener implementation that will handle a few more callbacks this time.
// They are again implemented below main.
class MyWindowListener : public ny::WindowListener {
public:
	ny::LoopControl* loopControl;
	ny::WindowContext* windowContext;
	ny::BufferSurface* bufferSurface;
	ny::ToplevelState toplevelState;
	nytl::Vec2ui windowSize {800u, 500u};

public:
	void draw(const ny::DrawEvent&) override;
	void mouseButton(const ny::MouseButtonEvent&) override;
	void key(const ny::KeyEvent&) override;
	void state(const ny::StateEvent&) override;
	void close(const ny::CloseEvent&) override;
	void resize(const ny::SizeEvent&) override;
};

int main()
{
	// The same setup as in the first (intro) example
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();
	auto listener = MyWindowListener {};

	ny::BufferSurface* bufferSurface {};
	auto ws = ny::WindowSettings {};

	ws.listener = &listener;
	ws.surface = ny::SurfaceType::buffer;
	ws.buffer.storeSurface = &bufferSurface;
	auto wc = ac->createWindowContext(ws);

	ny::LoopControl control {};
	listener.loopControl = &control;
	listener.windowContext = wc.get();
	listener.bufferSurface = bufferSurface;

	ny::log("Entering main loop");
	ac->dispatchLoop(control);
}

void MyWindowListener::draw(const ny::DrawEvent&)
{
	auto guard = bufferSurface->buffer();
	auto image = guard.get();
	auto size = ny::dataSize(image);
	std::memset(image.data, 0xFF, size); // opaque white
}

void MyWindowListener::key(const ny::KeyEvent& keyEvent)
{
	if(keyEvent.pressed) {
		auto keycode = keyEvent.keycode;
		if(keycode == ny::Keycode::f) {
			ny::log("f pressed. Toggling fullscreen");
			if(toplevelState == ny::ToplevelState::fullscreen) {
				windowContext->fullscreen();
				toplevelState = ny::ToplevelState::fullscreen;
			} else {
				windowContext->normalState();
			}
		} else if(keycode == ny::Keycode::n) {
			ny::log("n pressed. Resetting window to normal state");
			windowContext->normalState();
		} else if(keycode == ny::Keycode::escape) {
			ny::log("escape pressed. Closing window and exiting");
			loopControl->stop();
		} else if(keycode == ny::Keycode::m) {
			ny::log("m pressed. Toggle window maximize");
			if(toplevelState == ny::ToplevelState::maximized) {
				windowContext->maximize();
				toplevelState = ny::ToplevelState::maximized;
			} else {
				windowContext->normalState();
			}
		} else if(keycode == ny::Keycode::i) {
			ny::log("i pressed, Minimizing window");
			windowContext->minimize();
		} else if(keycode == ny::Keycode::d) {
			ny::log("d pressed. Trying to toggle decorations");
			windowContext->customDecorated(!windowContext->customDecorated());
		}
	}
}

void MyWindowListener::close(const ny::CloseEvent&)
{
	ny::log("Window was closed by server side. Exiting");
	loopControl->stop();
}

void MyWindowListener::mouseButton(const ny::MouseButtonEvent& event)
{
	if(event.pressed && event.button == ny::MouseButton::left) {
		if(event.position.x < 0 || event.position.y < 0 ||
			static_cast<unsigned int>(event.position.x) > windowSize.x ||
			static_cast<unsigned int>(event.position.y) > windowSize.y)
				return;

		ny::WindowEdges resizeEdges = ny::WindowEdge::none;
		if(event.position.x < 100)
			resizeEdges |= ny::WindowEdge::right;
		else if(static_cast<unsigned int>(event.position.x) > windowSize.x - 100)
			resizeEdges |= ny::WindowEdge::left;

		if(event.position.y < 100)
			resizeEdges |= ny::WindowEdge::top;
		else if(static_cast<unsigned int>(event.position.y) > windowSize.y - 100)
			resizeEdges |= ny::WindowEdge::bottom;

		if(resizeEdges != ny::WindowEdge::none) {
			ny::log("Starting to resize window");
			windowContext->beginResize(event.eventData, resizeEdges);
		} else {
			ny::log("Starting to move window");
			windowContext->beginMove(event.eventData);
		}
	}
}


void MyWindowListener::state(const ny::StateEvent& stateEvent)
{
	if(stateEvent.state != toplevelState)
		toplevelState = stateEvent.state;
}

void MyWindowListener::resize(const ny::SizeEvent& sizeEvent)
{
	windowSize = sizeEvent.size;
}
