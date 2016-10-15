#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/backend/integration/cairo.hpp>

#include <cairo/cairo.h>

//used in the moment to test data sources and data offers, dragndrop and clipboard stuff

//TODO:
// - wayland & x11: atm no clipboard and dnd support
// - winapi: does not work with std::any due to incorrect mingw/gcc linking of inline stuff

//Our CustomDataSource implementation that will be used if we want to provide data of different
//types to the backend, e.g. for the clipboard or dnd (drag-and-drop) operations.
class CustomDataSource : public ny::DataSource
{
	ny::DataTypes types() const override { return {{ny::dataType::text}}; }
	std::any data(unsigned int format) const override;
};


class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop, ny::WindowContext& wc) : lc_(mainLoop), wc_(wc) {}
	bool handleEvent(const ny::Event& ev) override;

	ny::AppContext* ac;
	ny::CairoIntegration* cairo;

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
	handler.ac = ac.get();

	auto cairo = ny::cairoIntegration(*wc); 
	if(!cairo)
	{
		ny::error("Failed to create cairo integration");
		return EXIT_FAILURE;
	}

	handler.cairo = cairo.get();

	// wc->droppable({{ny::dataType::text, ny::dataType::filePaths}});
	wc->eventHandler(handler);
	wc->refresh();

	ny::debug("Entering main loop");
	ac->dispatchLoop(control);
}

bool MyEventHandler::handleEvent(const ny::Event& ev)
{ 
	if(ev.type() == ny::eventType::close)
	{
		ny::debug("Window closed. Exiting.");
		lc_.stop();
	}
	else if(ev.type() == ny::eventType::dataOffer)
	{
		auto& offer = reinterpret_cast<const ny::DataOfferEvent&>(ev).offer;
		offer->data(ny::dataType::text, [](const std::any& text, const ny::DataOffer&, int) {
			if(!text.has_value()) return;
			ny::debug("Received dnd text data: ", std::any_cast<std::string>(text));
		});
	}
	else if(ev.type() == ny::eventType::mouseButton)
	{
		//Initiate a dnd operation with the CustomDataSource
		// ny::debug("Starting a dnd operation");
		// appContext_.startDragDrop(std::make_unique<CustomDataSource>());
	}
	else if(ev.type() == ny::eventType::draw)
	{
		//XXX: using cairo to draw onto the window
		//Here, get the surface guard which wraps a cairo_surface_t
		auto surfGuard = cairo->get();
		auto& surf = surfGuard.surface();

		//Then, create a cairo context for the returned surface and use it to draw
		auto cr = cairo_create(&surf);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(cr, 0.543, 0.4, 0.8, 0.5);
		cairo_paint(cr);

		//always remember to destroy/recreate the cairo context on every draw call and dont
		//store it since the cairo surface might change from call to call
		cairo_destroy(cr);

		return true;
	}
	else if(ev.type() == ny::eventType::key)
	{
		if(!static_cast<const ny::KeyEvent&>(ev).pressed) return false;

		//retrieving the clipboard DataOffer and listing all formats
		auto dataOffer = ac->clipboard();
		if(!dataOffer)
		{
			ny::error("Backend does not support clipboard operations...");
		}
		else
		{
			for(auto& t : dataOffer->types().types) ny::debug("clipboard type ", t);
 
			// trying to retrieve the data in text form and outputting it if successful
			dataOffer->data(ny::dataType::text, 
				[](const std::any& text, const ny::DataOffer&) {
					if(!text.has_value()) return;
					ny::debug("Received clipboard text data: ", std::any_cast<std::string>(text));
				});
		}

		ny::debug("aryy");
	}

	return false;
}

std::any CustomDataSource::data(unsigned int format) const 
{
	if(format != ny::dataType::text) return {};
	return std::string("ayyy got em");
}
