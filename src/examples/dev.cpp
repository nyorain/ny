#include <ny/ny.hpp>
#include <nytl/vecOps.hpp>

//used in the moment to test data sources and data offers, dragndrop and clipboard stuff

//Our CustomDataSource implementation that will be used if we want to provide data of different
//types to the backend, e.g. for the clipboard or dnd (drag-and-drop) operations.
class CustomDataSource : public ny::DataSource
{
	std::vector<ny::DataFormat> formats() const override { return {ny::DataFormat::text}; }
	std::any data(const ny::DataFormat& format) const override;
};

void handleDataOffer(ny::DataOffer& dataOffer)
{
	auto formatsRequest = dataOffer.formats();
	if(!formatsRequest)
	{
		ny::warning("could not retrieve formats request");
		return;
	}

	formatsRequest->wait();

	for(auto& fmt : formatsRequest->get()) ny::log("clipboard type ", fmt.name);

	// trying to retrieve the data in text form and outputting it if successful
	auto dataRequest = dataOffer.data(ny::DataFormat::text);
	if(!dataRequest)
	{
		ny::warning("could not retrieve data request");
		return;
	}

	dataRequest->wait();

	auto text = dataRequest->get();
	if(!text.has_value())
	{
		ny::warning("invalid text clipboard data offer");
		return;
	}

	ny::log("Received offer text data: ", std::any_cast<std::string>(text));
}

class MyWindowListener : public ny::WindowListener
{
public:
	ny::AppContext* ac;
	ny::WindowContext* wc;
	ny::LoopControl* lc;
	ny::BufferSurface* surface;

public:
	void close(const ny::EventData*) override
	{
		ny::log("Recevied closed event. Exiting");
		lc->stop();
	}

	void draw(const ny::EventData*) override
	{
		auto bufferGuard = surface->buffer();
		auto buffer = bufferGuard.get();
		auto size = buffer.stride * buffer.size.y;
		std::memset(buffer.data, 0xCC, size);
	}

	void mouseButton(bool pressed, ny::MouseButton button, const ny::EventData*) override
	{
		// initiate a dnd operation with the CustomDataSource
		if(nytl::anyOf(ac->mouseContext()->position() > 100)) return;
		auto ret = ac->startDragDrop(std::make_unique<CustomDataSource>());
		ny::log("Starting a dnd operation: ", ret);
	}

	void mouseWheel(float value, const ny::EventData*) override
	{
		ny::log("Mouse Wheel rotated: value=", value);
	}

	void dndDrop(nytl::Vec2i pos, ny::DataOfferPtr offer, const ny::EventData*) override
	{
		ny::log("offer event received");
		handleDataOffer(*offer);
	}

	void key(bool pressed, ny::Keycode keycode, const std::string& utf8,
		const ny::EventData*) override
	{
		if(keycode == ny::Keycode::escape)
		{
			ny::log("Escape pressed, exiting");
			lc->stop();
			return;
		}

		//retrieving the clipboard DataOffer and listing all formats
		ny::log("checking clipboard...");
		auto dataOffer = ac->clipboard();

		if(!dataOffer) ny::warning("Backend does not support clipboard operations...");
		else handleDataOffer(*dataOffer);
	}
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	ny::BufferSurface* bufferSurface {};
	MyWindowListener listener;

	ny::WindowSettings settings;
	settings.title = "Ayy sick shit";
	settings.listener = &listener;
	settings.surface = ny::SurfaceType::buffer;
	settings.buffer.storeSurface = &bufferSurface;
	auto wc = ac->createWindowContext(settings);

	ny::LoopControl control;

	listener.lc = &control;
	listener.ac = ac.get();
	listener.wc = wc.get();
	listener.surface = bufferSurface;

	wc->refresh();

	ny::log("Entering main loop");
	ac->dispatchLoop(control);
}

std::any CustomDataSource::data(const ny::DataFormat& format) const
{
	if(format != ny::DataFormat::text) return {};
	return std::string("ayyy got em");
}
