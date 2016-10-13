#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/base/key.hpp>
#include <ny/backend/integration/surface.hpp>

class MyEventHandler : public ny::EventHandler
{
public:
	ny::BufferSurface* buffer;

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

	wc->eventHandler(handler);
	wc->refresh();

	//XXX: the new part
	//we just create the surface for the WindowContext
	//in this case the type of the Surface should be a BufferSurface since we did
	//not request any context to be created for the WindowContext
	//Otherwise, the surface could have e.g. the types opengl context or vulkan surface
	auto surface = ny::surface(*wc);
	if(surface.type != ny::SurfaceType::buffer)
	{
		ny::error("Failed to create surface buffer integration");
		return EXIT_FAILURE;
	}

	handler.buffer = surface.buffer.get();

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
		auto guard = buffer->get();
		auto data = guard.get(); //decltype(data): ny::MutableImageData

		//we just fill the image data with 255 color values which should result in a white surface
		//note that doing so is really bad performance-wise (this might be a 1-million-time
		//executed for loop with explicit memory access) so the window might lag far behind
		//when resizing.
		//For a better example, see surface-cairo which uses cairo at this pointer for way
		//better performance (since cairo is optimized).
		auto size = ny::imageDataFormatSize(data.format);
		for(unsigned int i = 0; i < data.size.x * data.size.y * size; i += 1)
			data.data[i] = 0xFF;
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
