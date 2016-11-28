#include <ny/ny.hpp>
#include <nytl/vecOps.hpp>

//used in the moment to test data sources and data offers, dragndrop and clipboard stuff

//Our CustomDataSource implementation that will be used if we want to provide data of different
//types to the backend, e.g. for the clipboard or dnd (drag-and-drop) operations.
class CustomDataSource : public ny::DataSource
{
public:
	CustomDataSource();

	std::vector<ny::DataFormat> formats() const override { return {ny::DataFormat::text}; }
	std::any data(const ny::DataFormat& format) const override;
	ny::ImageData image() const override;

private:
	ny::OwnedImageData image_;
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
		nytl::unused(button);
		if(!pressed) return;

		// initiate a dnd operation with the CustomDataSource
		if(nytl::anyOf(ac->mouseContext()->position() > 100u)) return;
		auto ret = ac->startDragDrop(std::make_unique<CustomDataSource>());
		ny::log("Starting a dnd operation: ", ret);
	}

	void mouseWheel(float value, const ny::EventData*) override
	{
		ny::log("Mouse Wheel rotated: value=", value);
	}

	ny::DataFormat dndMove(nytl::Vec2i pos, const ny::DataOffer& offer,
		const ny::EventData*) override
	{
		if(nytl::allOf(pos > nytl::Vec2i(100, 100)) && nytl::allOf(pos < nytl::Vec2i(700, 400)))
		{
			auto formatsReq = offer.formats();
			formatsReq->wait();

			//XXX: soon
			// if(!formatsReq->wait())
			// {
			// 	ny::warning("AppContext broken!");
			// 	lc_.stop();
			// 	return;
			// }

			auto result = formatsReq->get();
			for(const auto& fmt : result) if(fmt == ny::DataFormat::text) return fmt;
		}

		return ny::DataFormat::none;
	}

	void dndDrop(nytl::Vec2i pos, ny::DataOfferPtr offer, const ny::EventData*) override
	{
		nytl::unused(pos);

		ny::log("offer event received");
		handleDataOffer(*offer);
	}

	void key(bool pressed, ny::Keycode keycode, const std::string& utf8,
		const ny::EventData*) override
	{
		nytl::unused(utf8);
		if(!pressed) return;

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

	ny::log("Entering main loop");
	ac->dispatchLoop(control);
}

CustomDataSource::CustomDataSource()
{
	image_.data = std::make_unique<std::uint8_t[]>(32 * 32 * 4);
	image_.format = ny::ImageDataFormat::rgba8888;
	image_.size = {32u, 32u};

	std::memset(image_.data.get(), 0xCC, 32 * 32 * 4);
}

std::any CustomDataSource::data(const ny::DataFormat& format) const
{
	if(format != ny::DataFormat::text) return {};
	return std::string("ayyy got em");
}

ny::ImageData CustomDataSource::image() const
{
	return {image_.data.get(), image_.size, image_.format, image_.stride};
}
