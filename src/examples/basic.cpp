// This time, we explicitly include only the needed headers
#include <ny/backend.hpp> // ny::Backend
#include <ny/appContext.hpp> // ny::AppContext
#include <ny/windowContext.hpp> // ny::WindowContext
#include <ny/windowListener.hpp> // ny::WindowListener
#include <ny/windowSettings.hpp> // ny::WindowSettings
#include <ny/keyboardContext.hpp> // ny::KeyboardContext
#include <ny/bufferSurface.hpp> // ny::BufferSurface
#include <ny/key.hpp> // ny::Keycode
#include <ny/mouseButton.hpp> // ny::MouseButton
#include <ny/image.hpp> // ny::Image
#include <ny/event.hpp> // ny::*Event
#include <dlg/dlg.hpp> // logging

#include <nytl/vecOps.hpp> // print nytl::Vec
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
	ny::AppContext* appContext;
	ny::WindowContext* windowContext;
	ny::BufferSurface* bufferSurface;
	ny::ToplevelState toplevelState;
	nytl::Vec2ui windowSize {800u, 500u};
	bool* run;

public:
	void draw(const ny::DrawEvent&) override;
	void mouseButton(const ny::MouseButtonEvent&) override;
	void key(const ny::KeyEvent&) override;
	void state(const ny::StateEvent&) override;
	void close(const ny::CloseEvent&) override;
	void resize(const ny::SizeEvent&) override;
	void focus(const ny::FocusEvent&) override;
	void surfaceCreated(const ny::SurfaceCreatedEvent&) override;
	void surfaceDestroyed(const ny::SurfaceDestroyedEvent&) override;
};

int main(int, char**)
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

	auto run = true;
	listener.appContext = ac.get();
	listener.windowContext = wc.get();
	listener.bufferSurface = bufferSurface;
	listener.appContext = ac.get();
	listener.run = &run;

	dlg_info("Entering main loop");
	while(run) {
		ac->waitEvents();
	}

	dlg_info("Returning from main with grace");
}

void MyWindowListener::draw(const ny::DrawEvent&)
{
	if(!bufferSurface) {
		dlg_info("draw: no bufferSurface");
		return;
	}

	auto guard = bufferSurface->buffer();
	auto image = guard.get();
	auto size = ny::dataSize(image);
	dlg_info("drawing the window: size {}", image.size);

	std::memset(image.data, 0xFF, size); // opaque white
}

void MyWindowListener::key(const ny::KeyEvent& keyEvent)
{
	std::string name = "<unknown>";
	if(appContext->keyboardContext()) {
		auto utf8 = appContext->keyboardContext()->utf8(keyEvent.keycode);
		if(!utf8.empty() && !ny::specialKey(keyEvent.keycode)) name = utf8;
		else name = "<unprintable>";
	}

	std::string_view utf8 = (keyEvent.utf8.empty() || ny::specialKey(keyEvent.keycode)) ?
		"<unprintable>" : keyEvent.utf8;
	dlg_info("Key {} with keycode ({}: {}) {}, generating: {} {}", name,
		(unsigned int) keyEvent.keycode, ny::name(keyEvent.keycode),
		keyEvent.pressed ? "pressed" : "released", utf8,
		keyEvent.repeat ? "(repeated)" : "");

	if(keyEvent.pressed) {
		auto keycode = keyEvent.keycode;
		if(keycode == ny::Keycode::f) {
			dlg_info("Toggling fullscreen");
			if(toplevelState != ny::ToplevelState::fullscreen) {
				windowContext->fullscreen();
				toplevelState = ny::ToplevelState::fullscreen;
			} else {
				windowContext->normalState();
				toplevelState = ny::ToplevelState::normal;
			}
		} else if(keycode == ny::Keycode::n) {
			dlg_info("Resetting window to normal state");
			windowContext->normalState();
		} else if(keycode == ny::Keycode::escape) {
			dlg_info("Closing window and exiting");
			*run = false;
		} else if(keycode == ny::Keycode::m) {
			dlg_info("Toggle window maximize");
			if(toplevelState != ny::ToplevelState::maximized) {
				windowContext->maximize();
				toplevelState = ny::ToplevelState::maximized;
			} else {
				windowContext->normalState();
				toplevelState = ny::ToplevelState::normal;
			}
		} else if(keycode == ny::Keycode::i) {
			dlg_info("Minimizing window");
			toplevelState = ny::ToplevelState::minimized;
			windowContext->minimize();
		} else if(keycode == ny::Keycode::d) {
			dlg_info("Trying to toggle decorations");
			windowContext->customDecorated(!windowContext->customDecorated());
			windowContext->refresh();
		}
	}
}

void MyWindowListener::close(const ny::CloseEvent&)
{
	dlg_info("Window was closed by server side. Exiting");
	*run = false;
}

void MyWindowListener::mouseButton(const ny::MouseButtonEvent& event)
{
	dlg_info("mouseButton {} {} at {}", ny::mouseButtonName(event.button),
		event.pressed ? "pressed" : "released", event.position);
	if(event.pressed && event.button == ny::MouseButton::left) {
		if(event.position[0] < 0 || event.position[1] < 0 ||
			static_cast<unsigned int>(event.position[0]) > windowSize[0] ||
			static_cast<unsigned int>(event.position[1]) > windowSize[1])
				return;

		ny::WindowEdges resizeEdges = ny::WindowEdge::none;
		if(event.position[0] < 100) {
			resizeEdges |= ny::WindowEdge::left;
		} else if(static_cast<unsigned int>(event.position[0]) > windowSize[0] - 100) {
			resizeEdges |= ny::WindowEdge::right;
		}

		if(event.position[1] < 100) {
			resizeEdges |= ny::WindowEdge::top;
		} else if(static_cast<unsigned int>(event.position[1]) > windowSize[1] - 100) {
			resizeEdges |= ny::WindowEdge::bottom;
		}

		auto caps = windowContext->capabilities();
		if(resizeEdges != ny::WindowEdge::none && caps & ny::WindowCapability::beginResize) {
			dlg_info("Starting to resize window");
			windowContext->beginResize(event.eventData, resizeEdges);
		} else if(caps & ny::WindowCapability::beginMove) {
			dlg_info("Starting to move window");
			windowContext->beginMove(event.eventData);
		}
	}
}

void MyWindowListener::focus(const ny::FocusEvent& ev)
{
	dlg_info("focus: {}", ev.gained);
}

void MyWindowListener::state(const ny::StateEvent& stateEvent)
{
	dlg_info("window state changed");
	if(stateEvent.state != toplevelState) {
		toplevelState = stateEvent.state;
	}
	windowContext->refresh();
}

void MyWindowListener::resize(const ny::SizeEvent& sizeEvent)
{
	dlg_info("window resized to {}", sizeEvent.size);
	windowSize = sizeEvent.size;
}

void MyWindowListener::surfaceCreated(const ny::SurfaceCreatedEvent& surfaceEvent)
{
	bufferSurface = surfaceEvent.surface.buffer;
	windowContext->refresh();
}

void MyWindowListener::surfaceDestroyed(const ny::SurfaceDestroyedEvent&)
{
	bufferSurface = nullptr;
}
