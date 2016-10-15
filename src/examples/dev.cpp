#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/backend/wayland/appContext.hpp>

#include <poll.h>
#include <unistd.h>

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

protected:
	ny::LoopControl& lc_;
	ny::WindowContext& wc_;
};


int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	//retrieving the clipboard DataOffer and listing all formats
	auto dataOffer = ac->clipboard();
	if(!dataOffer)
	{
		ny::error("Backend does not support clipboard operations...");
		// return EXIT_FAILURE;
	}
	else
	{
		for(auto& t : dataOffer->types().types) ny::debug("clipboard type ", t);

		//trying to retrieve the data in text form and outputting it if successful
		dataOffer->data(ny::dataType::text, [](const ny::DataOffer&, int, const std::any& text) {
			// if(!text.has_value()) return;
			ny::debug("Received clipboard text data: ", std::any_cast<std::string>(text));
		});

		//setting the clipboard data to the custom DataSource
		ny::debug("Setting clipboard data to 'ayyyy got em'");
		ny::debug("success: ", ac->clipboard(std::make_unique<CustomDataSource>()));
	}

	ny::WindowSettings settings;
	auto wc = ac->createWindowContext(settings);

	ny::LoopControl control;
	MyEventHandler handler(control, *wc);

	//DEBUG
	auto wlac = dynamic_cast<ny::WaylandAppContext*>(ac.get());
	wlac->fdCallback(STDIN_FILENO, POLLIN, [&]{ 
		std::cout << "input!\n";
		std::string a;
		std::cin >> a;
		if(a == "hi") std::cout << ">> hi \n";
		if(a == "exit") control.stop();
		else std::cout << ">> received " << a << "\n";
	});
	//DEBUG

	wc->droppable({{ny::dataType::text, ny::dataType::filePaths}});
	wc->eventHandler(handler);

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
		offer->data(ny::dataType::text, [](const ny::DataOffer&, int, const std::any& text) {
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

	return false;
}

std::any CustomDataSource::data(unsigned int format) const 
{
	if(format != ny::dataType::text) return {};
	return std::string("ayyy got em");
}
