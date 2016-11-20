#include <ny/ny.hpp>
#include <nytl/vecOps.hpp>

//used in the moment to test data sources and data offers, dragndrop and clipboard stuff

//TODO:
// - wayland & x11: atm no clipboard and dnd support
// - winapi: does not work with std::any due to incorrect mingw/gcc linking of inline stuff

//Our CustomDataSource implementation that will be used if we want to provide data of different
//types to the backend, e.g. for the clipboard or dnd (drag-and-drop) operations.
class CustomDataSource : public ny::DataSource
{
	std::vector<ny::DataFormat> formats() const override { return {ny::DataFormat::text}; }
	std::any data(const ny::DataFormat& format) const override;
};

class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc) : lc_(mainLoop), wc_(wc) {}
	bool handleEvent(const ny::Event& ev) override;

	ny::AppContext* ac;
	ny::BufferSurface* surface;

protected:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};


int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	ny::BufferSurface* bufferSurface {};

	ny::WindowSettings settings;
	// settings.initState = ny::ToplevelState::maximized;
	settings.title = "Ayy sick shit";
	settings.surface = ny::SurfaceType::buffer;
	settings.buffer.storeSurface = &bufferSurface;
	auto wc = ac->createWindowContext(settings);

	ny::LoopControl control;

	MyEventHandler handler(control, *wc);
	handler.ac = ac.get();
	handler.surface = bufferSurface;

	// wc->droppable({{ny::dataType::text, ny::dataType::filePaths}});
	wc->eventHandler(handler);
	wc->refresh();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}

void handleDataOffer(ny::DataOffer& dataOffer)
{
	auto formatsRequest = dataOffer.formats();
	formatsRequest->wait();

	for(auto& fmt : formatsRequest->get()) ny::debug("clipboard type ", fmt.name);

	// trying to retrieve the data in text form and outputting it if successful
	auto dataRequest = dataOffer.data(ny::DataFormat::text);
	dataRequest->wait();

	auto text = dataRequest->get();
	if(!text.has_value())
	{
		ny::warning("invalid text clipboard data offer");
		return;
	}

	ny::debug("Received clipboard text data: ", std::any_cast<std::string>(text));
}

bool MyEventHandler::handleEvent(const ny::Event& ev)
{
	ny::debug("event: ", ev.type());
	static std::unique_ptr<ny::DataOffer> offer;

	if(ev.type() == ny::eventType::close)
	{
		ny::debug("Window closed. Exiting.");
		lc_.stop();
		return true;
	}
	else if(ev.type() == ny::eventType::mouseWheel)
	{
		ny::debug("Mouse wheel: ", static_cast<const ny::MouseWheelEvent&>(ev).value);
		return true;
	}
	else if(ev.type() == ny::eventType::dataOffer)
	{
		ny::debug("offer event received");
		handleDataOffer(*reinterpret_cast<const ny::DataOfferEvent&>(ev).offer);
		return true;
	}
	else if(ev.type() == ny::eventType::mouseButton)
	{
		// initiate a dnd operation with the CustomDataSource
		if(nytl::anyOf(static_cast<const ny::MouseButtonEvent&>(ev).position > 100))
			return false;

		auto ret = ac->startDragDrop(std::make_unique<CustomDataSource>());
		ny::debug("Starting a dnd operation: ", ret);
		return true;
	}
	else if(ev.type() == ny::eventType::draw)
	{
		auto bufferGuard = surface->buffer();
		auto buffer = bufferGuard.get();

		auto size = buffer.stride * buffer.size.y;
		std::memset(buffer.data, 0xCC, size);

		return true;
	}
	else if(ev.type() == ny::eventType::key)
	{
		auto& kev = static_cast<const ny::KeyEvent&>(ev);
		if(!kev.pressed) return false;

		if(kev.keycode == ny::Keycode::escape)
		{
			ny::debug("Escape pressed, exiting");
			lc_.stop();
			return true;
		}

		//retrieving the clipboard DataOffer and listing all formats
		ny::debug("checking clipboard...");
		auto dataOffer = ac->clipboard();
		if(!dataOffer) ny::warning("Backend does not support clipboard operations...");
		else handleDataOffer(*dataOffer);

		return true;
	}

	return false;
}

std::any CustomDataSource::data(const ny::DataFormat& format) const
{
	if(format != ny::DataFormat::text) return {};
	return std::string("ayyy got em");
}
