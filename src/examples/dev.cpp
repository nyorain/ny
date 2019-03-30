#include <ny/ny.hpp>
#include <nytl/vecOps.hpp>
#include <dlg/dlg.hpp>
#include <any>

// used at the moment to test data sources and data offers, dragndrop and clipboard stuff

// Our CustomDataSource implementation that will be used if we want to provide data of different
// types to the backend, e.g. for the clipboard or dnd (drag-and-drop) operations.
class CustomDataSource : public ny::DataSource {
public:
	CustomDataSource();

	std::vector<ny::DataFormat> formats() const override { return {ny::DataFormat::text}; }
	std::any data(const ny::DataFormat& format) const override;
	ny::Image image() const override;

private:
	ny::UniqueImage image_;
};

/// Returns whether AppContext received error
bool handleDataOffer(ny::DataOffer& dataOffer)
{
	auto formatsRequest = dataOffer.formats();
	if(!formatsRequest) {
		dlg_warn("could not retrieve formats request");
		return true;
	}

	if(!formatsRequest->wait()) {
		dlg_warn("AppContext broke while waiting for formats! Exiting.");
		return false;
	}

	auto formats = formatsRequest->get();

	ny::DataFormat dataFormat {};
	for(const auto& fmt : formats) {
		dlg_info("clipboard type {}", fmt.name);
		if(fmt == ny::DataFormat::text) {
			dataFormat = fmt;
			break;
		} else if(fmt == ny::DataFormat::uriList && dataFormat == ny::DataFormat::none) {
			dataFormat = fmt;
		}
	}

	if(dataFormat == ny::DataFormat::none) {
		dlg_info("no supported clipboard format!");
		return true;
	}

	// trying to retrieve the data in text form and outputting it if successful
	auto dataRequest = dataOffer.data(dataFormat);
	if(!dataRequest) {
		dlg_warn("could not retrieve data request");
		return true;
	}

	if(!dataRequest->wait()) {
		dlg_warn("AppContext broke while waiting for data! Exiting.");
		return false;
	}

	auto data = dataRequest->get();
	if(!data.has_value()) {
		dlg_warn("invalid text clipboard data offer");
		return true;
	}

	if(dataFormat == ny::DataFormat::text) {
		dlg_info("Received offer text data: {}", std::any_cast<const std::string&>(data));
	} else if(dataFormat == ny::DataFormat::uriList) {
		auto uriList = std::any_cast<const std::vector<std::string>&>(data);
		for(auto& uri : uriList)
			dlg_info("Received offer uri: {}", uri);
	}

	return true;
}

class MyWindowListener : public ny::WindowListener {
public:
	ny::AppContext* ac;
	ny::WindowContext* wc;
	ny::BufferSurface* surface {};
	bool* run;

public:
	void close(const ny::CloseEvent&) override
	{
		dlg_info("Recevied closed event. Exiting");
		*run = false;
		ac->wakeupWait();
	}

	void draw(const ny::DrawEvent&) override
	{
		if(!surface) return;

		auto bufferGuard = surface->buffer();
		auto buffer = bufferGuard.get();
		auto size = dataSize(buffer);
		std::memset(buffer.data, 0x00, size);
	}

	void mouseButton(const ny::MouseButtonEvent& ev) override
	{
		if(!ev.pressed) return;

		// initiate a dnd operation with the CustomDataSource
		auto pos = ev.position;
		if(pos[0] < 100 && pos[1] < 100) {
			auto src = std::make_unique<CustomDataSource>();
			auto ret = ac->startDragDrop(std::move(src));
			dlg_info("Starting a dnd operation: {}", ret);
		} else if(pos[0] > 400 && pos[1] > 400) {
			wc->beginResize(ev.eventData, ny::WindowEdge::bottomRight);
		} else {
			wc->beginMove(ev.eventData);
		}
	}

	void mouseWheel(const ny::MouseWheelEvent& ev) override
	{
		dlg_info("Mouse Wheel rotated: value={}", ev.value);
	}

	ny::DataFormat dndMove(const ny::DndMoveEvent& ev) override
	{
		auto pos = ev.position;
		if(pos[0] > 100 && pos[1] > 100 && pos[0] < 700 && pos[1] < 400) {
			auto formatsReq = ev.offer->formats();
			if(!formatsReq->wait()) {
				dlg_warn("AppContext broke while waiting for formats! Exiting.");
				*run = false;
				ac->wakeupWait();
				return ny::DataFormat::none;
			}

			auto result = formatsReq->get();
			for(const auto& fmt : result)
				if(fmt == ny::DataFormat::text || fmt == ny::DataFormat::uriList)
					return fmt;
		}

		return ny::DataFormat::none;
	}

	void dndDrop(const ny::DndDropEvent& ev) override
	{
		if(!handleDataOffer(*ev.offer)) {
			*run = false;
			ac->wakeupWait();
		}
	}

	void key(const ny::KeyEvent& ev) override
	{
		if(!ev.pressed) return;

		if(ev.keycode == ny::Keycode::escape) {
			dlg_info("Escape pressed, exiting");
			*run = false;
			ac->wakeupWait();
			return;
		} else if(ev.keycode == ny::Keycode::c) {
			dlg_info("setting clipboard... ");
			if(ac->clipboard(std::make_unique<CustomDataSource>())) {
				dlg_info("\tsuccesful!");
			} else {
				dlg_info("\tunsuccesful");
			}
		}
		if(ev.keycode == ny::Keycode::v)
		{
			dlg_info("reading clipboard... ");
			auto dataOffer = ac->clipboard();

			if(!dataOffer) {
				dlg_info("Backend does not support clipboard operations...");
			} else if(!handleDataOffer(*dataOffer)) {
				*run = false;
				ac->wakeupWait();
			}
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
	settings.droppable = true;
	settings.listener = &listener;
	settings.surface = ny::SurfaceType::buffer;
	settings.buffer.storeSurface = &bufferSurface;
	settings.transparent = false;
	auto wc = ac->createWindowContext(settings);

	auto run = true;
	listener.run = &run;
	listener.ac = ac.get();
	listener.wc = wc.get();
	listener.surface = bufferSurface;

	dlg_info("Entering main loop");
	while(run) {
		ac->waitEvents();
	}
}

CustomDataSource::CustomDataSource()
{
	image_.data = std::make_unique<std::uint8_t[]>(32 * 32 * 4);
	image_.format = ny::ImageFormat::argb8888;
	image_.size = {32u, 32u};

	// light transparent red
	auto color = 0xAAFF6666;
	// auto color = 0xFFFFFFFF;

	auto it = reinterpret_cast<std::uint32_t*>(image_.data.get());
	std::fill(it, it + (32 * 32), color);
}

std::any CustomDataSource::data(const ny::DataFormat& format) const
{
	if(format != ny::DataFormat::text) {
		dlg_error("Invalid data format requested");
		return {};
	}

	std::any ret = std::string("ayy got em");
	return ret;
}

ny::Image CustomDataSource::image() const
{
	return image_;
}
