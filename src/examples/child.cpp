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

// The WindowListener implementation that will handle a few more callbacks this time.
// They are again implemented below main.
class MyWindowListener : public ny::WindowListener {
public:
	ny::AppContext* appContext;
	ny::WindowContext* windowContext;
	ny::BufferSurface* bufferSurface;
	ny::ToplevelState toplevelState;
	unsigned int color = 0x88;
	bool* run;

public:
	void draw(const ny::DrawEvent&) override;
	void key(const ny::KeyEvent&) override;
	void close(const ny::CloseEvent&) override;
	void mouseButton(const ny::MouseButtonEvent&) override;
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
	auto wc1 = ac->createWindowContext(ws);

	auto run = true;
	listener.appContext = ac.get();
	listener.windowContext = wc1.get();
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

	dlg_info("drawing window {}: {}", windowContext, image.size);
	std::memset(image.data, color, size);
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
		if(keycode == ny::Keycode::escape) {
			dlg_info("Closing window and exiting");
			if(run) {
				*run = false;
			} else {
				// TODO: allowed per spec?
				delete windowContext;	
				delete this;
			}
		}
	}
}

void MyWindowListener::mouseButton(const ny::MouseButtonEvent& mev) 
{
	if(mev.pressed && mev.button == ny::MouseButton::right) {
		ny::WindowSettings ws;
		auto listener = new MyWindowListener();
		ny::BufferSurface* bufferSurface {};
		ws.listener = listener;
		ws.parent = windowContext->nativeHandle();
		ws.surface = ny::SurfaceType::buffer;
		ws.buffer.storeSurface = &bufferSurface;
		ws.size = {400u, 150u};
		ws.position = {200u, 100u};
		auto wc2 = appContext->createWindowContext(ws);
		listener->appContext = appContext;
		listener->windowContext = wc2.get();
		listener->run = nullptr;
		listener->color = 0xCC;
		listener->bufferSurface = bufferSurface;
		wc2.release();
	}
}

void MyWindowListener::close(const ny::CloseEvent&)
{
	dlg_info("Window was closed by server side. Exiting");
	if(run) {
		*run = false;
	}
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