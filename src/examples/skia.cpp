#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/backend/integration/surface.hpp>

#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkImage.h>
#include <SkPath.h>
#include <SkSurface.h>

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

	std::printf("Jan stinkt\b\b\b\b\b\bist geil\n");

	ny::WindowSettings settings;
	auto wc = ac->createWindowContext(settings);

	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	wc->eventHandler(handler);
	wc->refresh();

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
		auto guard = buffer->get();
		auto data = guard.get(); //decltype(data): ny::MutableImageData

		SkImageInfo info = SkImageInfo::Make(data.size.x, data.size.y, kBGRA_8888_SkColorType, 
			kPremul_SkAlphaType);
		size_t rowBytes = info.minRowBytes();
		ny::debug("rowBytes: ", rowBytes);
		ny::debug("data: ", (void*)data.data);

		auto surface = SkSurface::MakeRasterDirect(info, data.data, data.stride);
		SkCanvas* canvas = surface->getCanvas();

		const SkScalar scale = 256.0f;
		const SkScalar R = 0.45f * scale;
		const SkScalar TAU = 6.2831853f;
		SkPath path;
		for (int i = 0; i < 5; ++i) 
		{
			SkScalar theta = 2 * i * TAU / 5;
			if (i == 0) {
				path.moveTo(R * cos(theta), R * sin(theta));
			} else {
				path.lineTo(R * cos(theta), R * sin(theta));
			}
		}

		path.close();
		SkPaint p;
		p.setAntiAlias(true);
		canvas->clear(SK_ColorWHITE);
		canvas->translate(0.5f * scale, 0.5f * scale);
		canvas->drawPath(path, p);
	}
	else if(ev.type() == ny::eventType::key)
	{
		const auto& kev = static_cast<const ny::KeyEvent&>(ev);
		if(!kev.pressed) return false;

		if(kev.key == ny::Key::escape)
		{
			ny::debug("Esc key pressed. Exiting.");
			lc_.stop();
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
