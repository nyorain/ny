#include <ny/ny.hpp>

///XXX: this example shows how to draw into a raw memory buffer to display content in a window.
///This way one can easily use all software rasterizer do draw onto a ny window without having
///to call any platform-dependent code.

class MyEventHandler : public ny::EventHandler
{
public:
	ny::BufferSurface* surface;

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

	//XXX: the new part
	//Request a buffer surface to be created for the windowContext
	ny::BufferSurface* bufferSurface {};

	ny::WindowSettings settings;
	settings.surface = ny::SurfaceType::buffer;
	settings.buffer.storeSurface = &bufferSurface;
	auto wc = ac->createWindowContext(settings);

	if(!bufferSurface)
	{
		ny::error("Failed to create surface buffer integration");
		return EXIT_FAILURE;
	}

	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	wc->eventHandler(handler);
	wc->refresh();

	handler.surface = bufferSurface;

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
		//XXX: we get a buffer guard from the BufferSurface
		//and retrieve the buffer from its guard
		auto guard = surface->buffer();
		auto buffer = guard.get(); //decltype(data): ny::MutableImageData

		//we just fill the image data with 255 color values which should result in a white surface
		//note that doing so is really bad performance-wise (this might be a 1-million-time
		//executed for loop with explicit memory access) so the window might lag far behind
		//when resizing.
		//note than ny::ImageData specifies that stride=0 equals stride=size.x * formatsize
		auto size = buffer.stride * buffer.size.y;
		if(!size) size = buffer.size.x * ny::imageDataFormatSize(buffer.format) * buffer.size.y;

		std::memset(buffer.data, 0xCC, size);
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
};
