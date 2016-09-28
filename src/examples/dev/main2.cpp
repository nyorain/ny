#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/backend/integration/surface.hpp>
#include <ny/backend/winapi/windowContext.hpp>

class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc)
		: loopControl_(mainLoop), wc_(wc) {}

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
			auto guard = buffer_->get();
			auto data = guard.get();

			for(unsigned int i = 2; i < data.size.x * data.size.y * 4; i += 4)
				data.data[i] = 0xFF;
		}
		else if(ev.type() == ny::eventType::key)
		{
			const auto& kev = static_cast<const ny::KeyEvent&>(ev);
			if(!kev.pressed) return false;

			if(kev.key == ny::Key::escape)
			{
				ny::debug("Esc key pressed. Exiting.");
				loopControl_.stop();
				return true;
			}
			else if(kev.key == ny::Key::f)
			{
				ny::debug("f key pressed. Fullscreen.");
				wc_.fullscreen();
				return true;
			}
			else if(kev.key == ny::Key::n)
			{
				ny::debug("n key pressed. Normal.");
				wc_.normalState();
				return true;
			}
			else if(kev.key == ny::Key::i)
			{
				ny::debug("i key pressed. Iconic (minimize).");
				wc_.minimize();
				return true;
			}
			else if(kev.key == ny::Key::m)
			{
				ny::debug("m key pressed. maximize.");
				wc_.maximize();
				return true;
			}
		}

		return false;
	};

public:
	ny::BufferSurface* buffer_;
	ny::LoopControl& loopControl_;
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

	ny::DataTypes types {{ny::dataType::image, ny::dataType::filePaths}};
	wc->droppable(types);

	wc->eventHandler(handler);
	wc->refresh();

	auto surface = ny::surface(*wc);
	if(surface.type != ny::SurfaceType::buffer)
	{
		ny::warning("Failed to create surface buffer integration");
		return EXIT_FAILURE;
	}

	handler.buffer_ = surface.buffer.get();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}
