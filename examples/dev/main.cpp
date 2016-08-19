#include <ny/base.hpp>
#include <ny/backend.hpp>
#include <ny/app/events.hpp>
#include <cstdio>

//TODO:
// - remove WindowHint::acceptDrop and implement droppable for winapi instead
// - winapi dnd cursors/previews
// - winapi: setting the clipboard fails
// - winapi: gdi, vulkan, cairo, gl integration

// - wayland & x11: clipboard and dnd support

class MyEventHandler : public ny::EventHandler
{
public:
	MyEventHandler(ny::LoopControl& mainLoop) : loopControl_(mainLoop) {}

	bool handleEvent(const ny::Event& ev) override
	{
		if(ev.type() == ny::eventType::windowClose)
		{
			ny::debug("Window closed. Exiting.");
			loopControl_.stop();
		}

		if(ev.type() == ny::eventType::dataOffer)
		{
			reinterpret_cast<const ny::DataOfferEvent&>(ev).offer->data(ny::dataType::text,
				[](const ny::DataOffer&, int format, const std::any& text) {
					if(!text.has_value()) return;
					ny::debug("Received dnd text data: ", std::any_cast<std::string>(text));
				});
		}

		return false;
	};

protected:
	ny::LoopControl& loopControl_;
};

class CustomDataSource : public ny::DataSource
{
public:
	ny::DataTypes types() const override { return {{ny::dataType::text}}; }
	std::any data(unsigned int format) const override
	{
		if(format != ny::dataType::text) return {};
		return "ayyy got em";
	}
};

int main()
{
	auto& backend = ny::Backend::choose();
	auto ac = backend.createAppContext();

	auto dataOffer = ac->clipboard();
	dataOffer->data(ny::dataType::text,
		[](const ny::DataOffer&, int format, const std::any& text) {
			if(!text.has_value()) return;
			ny::debug("Received clipboard text data: ", std::any_cast<std::string>(text));
		});

	ny::debug("Setting clipboard data to 'ayyyy got em'");
	ny::debug("success: ", ac->clipboard(std::make_unique<CustomDataSource>()));

	ny::WindowSettings settings;
	auto wc = ac->createWindowContext(settings);

	ny::EventDispatcher dispatcher;
	ny::LoopControl control;
	MyEventHandler handler(control);

	wc->addWindowHints(ny::WindowHint::acceptDrop);
	wc->eventHandler(handler);
	ac->dispatchLoop(dispatcher, control);
}
