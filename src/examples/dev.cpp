#include <ny/ny.hpp>
#include <nytl/vecOps.hpp>
#include <dlg/dlg.hpp>
#include <thread>
#include <future>
#include <chrono>

// TODO: handle supported actions of data offer

// weston-dnd test
constexpr const char* mimeFlower = "application/x-wayland-dnd-flower";

// used at the moment to test data sources and data offers, dragndrop and clipboard stuff

// Our CustomDataSource implementation that will be used if we want to provide data of different
// types to the backend, e.g. for the clipboard or dnd (drag-and-drop) operations.
class CustomDataSource : public ny::DataSource {
public:
	CustomDataSource();

	std::vector<std::string> formats() const override { return {ny::mime::utf8}; }
	ny::ExchangeData data(nytl::StringParam format) const override;
	ny::Image image() const override;

private:
	ny::UniqueImage image_;
};

std::tuple<std::string, ny::DndAction>
chooseFormat(nytl::Span<const std::string> formats) {
	std::string format;
	auto best = 0u;
	auto action = ny::DndAction::copy;
	for(auto& fmt : formats) {
		if(fmt == ny::mime::utf8 && best < 1) {
			format = fmt;
			best = 1;
		} else if(fmt == ny::mime::uriList && best < 2) {
			format = fmt;
			best = 2;
		} else if(fmt == mimeFlower && best < 3) {
			action = ny::DndAction::move;
			format = fmt;
			best = 3;
		}
	}

	return {format, action};
}

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
			auto& text = std::get<std::string>(data);
			dlg_info("Received offer text data: {}", text);
		} else if(dataFormat == ny::mime::uriList) {
			auto& uriList = std::get<std::vector<std::string>>(data);
			for(auto& uri : uriList) {
				dlg_info("Received offer uri: {}", uri);
			}
		} else {
			dlg_info("Received data of type {}", dataFormat);
		}

		// requests completed, wake up waitEvents
		wait = false;
	};

	auto formatHandler = [&](const auto& formats){
		if(formats.empty()) {
			dlg_warn("clipboard formats request failed");
			wait = false;
			return;
		}

		auto [fmt, _] = chooseFormat(formats);
		dataFormat = fmt;

		// if we have a format, send data request
		if(!dataFormat.empty()) {
			dlg_info("requesting data: {}", dataFormat);
			dataOffer.data(dataFormat.c_str(), dataHandler);
		}
	};

	dataOffer.formats(formatHandler);
	while(wait) {
		// from the moment this is called for the first time the
		// dataOffer reference might be dangling and should not
		// longer be accessed (at least if this was invoked with the
		// non-owned clipboard data offer, see ny/appContext.hpp)
		ac.waitEvents();
	}
}

class MyWindowListener : public ny::WindowListener {
public:
	ny::AppContext* ac;
	ny::WindowContext* wc;
	ny::BufferSurface* surface {};
	bool* run;
	std::future<void> wakeupThread;

public:
	void close(const ny::CloseEvent&) override {
		dlg_info("Recevied closed event. Exiting");
		*run = false;
	}

	void draw(const ny::DrawEvent&) override {
		if(!surface) {
			return;
		}

		auto bufferGuard = surface->buffer();
		auto buffer = bufferGuard.get();
		auto size = dataSize(buffer);
		std::memset(buffer.data, 0xCC, size);
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

	void dndMove(const ny::DndMoveEvent& ev) override {
		auto formatHandler = [offer = ev.offer](const auto& flist) {
			auto [format, action] = chooseFormat(flist);
			offer->preferred(format, action);
		};

		// check the formats and choose
		ev.offer->formats(formatHandler);
	}

	void dndDrop(const ny::DndDropEvent& ev) override {
		// TODO: make sure we use the format we set in `preferred`
		// - maybe add (action) parameter in `data` and then
		//   make backends call preferred implictly one last
		//   time (if needed)?
		handleDataOffer(*ac, *ev.offer);
	}

	void key(const ny::KeyEvent& ev) override {
		if(!ev.pressed) {
			return;
		}

		if(ev.keycode == ny::Keycode::escape) {
			dlg_info("Escape pressed, exiting");
			*run = false;
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

		if(ev.keycode == ny::Keycode::w) {
			wakeupThread = std::async(std::launch::async, [&]{
				// wait some time then wakeup main loop
				// test that it works
				std::this_thread::sleep_for(std::chrono::seconds(1));
				ac->wakeupWait();
			});
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
		dlg_trace("main loop iteration");
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

ny::ExchangeData CustomDataSource::data(nytl::StringParam format) const {
	if(format != ny::mime::utf8) {
		dlg_error("Invalid data format requested");
		return {};
	}

	return {std::string("ayy got em")};
}

ny::Image CustomDataSource::image() const {
	return image_;
}
