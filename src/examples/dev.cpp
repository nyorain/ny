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

	std::vector<std::string> formats() const override { return {ny::mime::utf8}; }
	ny::ExchangeData data(const char* format) const override;
	ny::Image image() const override;

private:
	ny::UniqueImage image_;
};

/// Returns whether AppContext received error
void handleDataOffer(ny::AppContext& ac, ny::DataOffer& dataOffer) {
	// TODO: we could provide a wrapper in ny/dataExchange.hpp that does exactly this
	// blocking/only handling specific formats
	bool wait = true;
	std::string dataFormat;
	auto dataHandler = [&](const auto& data) {
		if(data.index() == 0) { // monstate; invalid
			dlg_warn("clipboard data request failed");
		} else if(dataFormat == ny::mime::utf8) {
			auto& text = std::get<const std::string&>(data);
			dlg_info("Received offer text data: {}", text);
		} else if(dataFormat == ny::mime::uriList) {
			auto& uriList = std::get<const std::vector<std::string>&>(data);
			for(auto& uri : uriList) {
				dlg_info("Received offer uri: {}", uri);
			}
		}

		// requests completed, wake up waitEvents
		wait = false;
		ac.wakeupWait();
	};

	auto formatHandler = [&](const auto& formats){
		if(formats.empty()) {
			dlg_warn("clipboard formats request failed");
			wait = false;
			ac.wakeupWait();
			return;
		}

		for(const auto& fmt : formats) {
			dlg_info("clipboard type {}", fmt);
			if(fmt == ny::mime::utf8 && dataFormat.empty()) {
				dataFormat = fmt;
			} else if(fmt == ny::mime::uriList) {
				dataFormat = fmt;
			}
		}

		// if we have a format, send data request
		if(!dataFormat.empty()) {
			dataOffer.data(dataFormat.c_str(), dataHandler);
		}
	};

	dataOffer.formats(formatHandler);
	while(wait) {
		ac.waitEvents();
	}
}

class MyWindowListener : public ny::WindowListener {
public:
	ny::AppContext* ac;
	ny::WindowContext* wc;
	ny::BufferSurface* surface {};
	bool* run;

public:
	void close(const ny::CloseEvent&) override {
		dlg_info("Recevied closed event. Exiting");
		*run = false;
		ac->wakeupWait();
	}

	void draw(const ny::DrawEvent&) override {
		if(!surface) {
			return;
		}

		auto bufferGuard = surface->buffer();
		auto buffer = bufferGuard.get();
		auto size = dataSize(buffer);
		std::memset(buffer.data, 0x00, size);
	}

	void mouseButton(const ny::MouseButtonEvent& ev) override {
		if(!ev.pressed) {
			return;
		}

		// initiate a dnd operation with the CustomDataSource
		auto pos = ev.position;
		if(pos[0] < 100 && pos[1] < 100) {
			auto src = std::make_unique<CustomDataSource>();
			auto ret = ac->dragDrop(ev.eventData, std::move(src));
			dlg_info("Starting a dnd operation: {}", ret);
		} else if(pos[0] > 400 && pos[1] > 400) {
			wc->beginResize(ev.eventData, ny::WindowEdge::bottomRight);
		} else {
			wc->beginMove(ev.eventData);
		}
	}

	void mouseWheel(const ny::MouseWheelEvent& ev) override {
		dlg_info("Mouse Wheel rotated: value={}", ev.value);
	}

	ny::DndResponse dndMove(const ny::DndMoveEvent& ev) override {
		std::vector<std::string> formats;
		bool wait = true;
		auto formatHandler = [&](const auto& flist) {
			for(auto& fmt : flist) {
				if(fmt == ny::mime::utf8 || fmt == ny::mime::uriList) {
					formats.push_back(fmt);
				}
			}

			wait = false;
			ac->wakeupWait();
		};

		ev.offer->formats(formatHandler);
		while(wait) {
			ac->waitEvents();
		}

		return {formats, ny::DndAction::copy};
	}

	void dndDrop(const ny::DndDropEvent& ev) override {
		handleDataOffer(*ac, *ev.offer);
	}

	void key(const ny::KeyEvent& ev) override {
		if(!ev.pressed) {
			return;
		}

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

		if(ev.keycode == ny::Keycode::v) {
			dlg_info("reading clipboard... ");
			auto dataOffer = ac->clipboard();

			if(!dataOffer) {
				dlg_info("Backend does not support clipboard operations...");
			} else {
				handleDataOffer(*ac, *dataOffer);
			}
		}
	}
};

int main() {
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

CustomDataSource::CustomDataSource() {
	image_.data = std::make_unique<std::byte[]>(32 * 32 * 4);
	image_.format = ny::ImageFormat::argb8888;
	image_.size = {32u, 32u};

	// light transparent red
	auto color = 0xAAFF6666;
	// auto color = 0xFFFFFFFF;

	auto it = reinterpret_cast<std::uint32_t*>(image_.data.get());
	std::fill(it, it + (32 * 32), color);
}

ny::ExchangeData CustomDataSource::data(const char* format) const {
	if(format != ny::mime::utf8) {
		dlg_error("Invalid data format requested");
		return {};
	}

	return {std::string("ayy got em")};
}

ny::Image CustomDataSource::image() const {
	return image_;
}
