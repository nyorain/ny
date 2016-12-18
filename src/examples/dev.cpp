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
	ny::Image image() const override;

private:
	ny::UniqueImage image_;
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
	auto formats = formatsRequest->get();

	ny::DataFormat dataFormat {};
	for(const auto& fmt : formats)
	{
		ny::log("clipboard type ", fmt.name);

		if(fmt == ny::DataFormat::text)
		{
			dataFormat = fmt;
			break;
		}

		if(fmt == ny::DataFormat::uriList && dataFormat == ny::DataFormat::none)
		{
			dataFormat = fmt;
		}
	}

	if(dataFormat == ny::DataFormat::none)
	{
		ny::log("no supported clipboard format!");
		return;
	}

	// trying to retrieve the data in text form and outputting it if successful
	auto dataRequest = dataOffer.data(dataFormat);
	if(!dataRequest)
	{
		ny::warning("could not retrieve data request");
		return;
	}

	dataRequest->wait();

	auto data = dataRequest->get();
	if(!data.has_value())
	{
		ny::warning("invalid text clipboard data offer");
		return;
	}

	if(dataFormat == ny::DataFormat::text)
	{
		ny::log("Received offer text data: ", std::any_cast<const std::string&>(data));
	}
	else if(dataFormat == ny::DataFormat::uriList)
	{
		auto uriList = std::any_cast<const std::vector<std::string>&>(data);
		for(auto& uri : uriList) ny::log("Received offer uri: ", uri);
	}
}

class MyWindowListener : public ny::WindowListener
{
public:
	ny::AppContext* ac;
	ny::WindowContext* wc;
	ny::LoopControl* lc;
	ny::BufferSurface* surface {};

public:
	void close(const ny::EventData*) override
	{
		ny::log("Recevied closed event. Exiting");
		lc->stop();
	}

	void draw(const ny::EventData*) override
	{
		if(!surface) return;

		auto bufferGuard = surface->buffer();
		auto buffer = bufferGuard.get();
		auto size = dataSize(buffer);
		std::memset(buffer.data, 0xCC, size);
	}

	void mouseButton(bool pressed, ny::MouseButton button, const ny::EventData* data) override
	{
		nytl::unused(button);
		if(!pressed) return;

		// initiate a dnd operation with the CustomDataSource
		if(nytl::allOf(ac->mouseContext()->position() < 100u))
		{
			auto src = std::make_unique<CustomDataSource>();
			auto ret = ac->startDragDrop(std::move(src));
			ny::log("Starting a dnd operation: ", ret);
		}
		else if(nytl::allOf(ac->mouseContext()->position() > 400u))
		{
			wc->beginResize(data, ny::WindowEdge::bottomRight);
		}
		else
		{
			wc->beginMove(data);
		}
	}

	void mouseWheel(float value, const ny::EventData*) override
	{
		ny::log("Mouse Wheel rotated: value=", value);
	}

	ny::DataFormat dndMove(nytl::Vec2i pos, ny::DataOffer& offer,
		const ny::EventData*) override
	{
		// ny::log("dnd pos: ", pos);
		if(nytl::allOf(pos > nytl::Vec2i(100, 100)) && nytl::allOf(pos < nytl::Vec2i(700, 400)))
		{
			// ny::log("dnd pos match");
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
			for(const auto& fmt : result)
			{
				// ny::log("format offer: ", fmt.name);
				if(fmt == ny::DataFormat::text || fmt == ny::DataFormat::uriList) return fmt;
			}
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
		if(keycode == ny::Keycode::c)
		{
			ny::log("setting clipboard... ");
			if(ac->clipboard(std::make_unique<CustomDataSource>())) ny::log("\tsuccesful!");
			else ny::log("\tunsuccesful");
		}
		if(keycode == ny::Keycode::v)
		{
			ny::log("reading clipboard... ");
			auto dataOffer = ac->clipboard();

			if(!dataOffer) ny::log("Backend does not support clipboard operations...");
			else handleDataOffer(*dataOffer);
		}
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
	image_.format = ny::imageFormats::argb8888;
	image_.size = {32u, 32u};

	//light transparent red
	auto color = 0xAAFF6666;

	auto it = reinterpret_cast<std::uint32_t*>(image_.data.get());
	std::fill(it, it + (32 * 32), color);

	// std::memset(image_.data.get(), 0xCC, 32 * 32 * 4);
}

std::any CustomDataSource::data(const ny::DataFormat& format) const
{
	if(format != ny::DataFormat::text) return {};
	return std::string("ayyy got em");
}

ny::Image CustomDataSource::image() const
{
	return image_;
}
