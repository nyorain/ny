#include <iostream>
#include <ny/ny.hpp>
#include <ny/backend/appContext.hpp>
#include <ny/base/data.hpp>
#include <nytl/time.hpp>
//#include <ny/backend/winapi/appContext.hpp>
//#include <ny/backend/winapi/windowContext.hpp>
//#include <ny/backend/winapi/gdi.hpp>

class MyHandler : public ny::EventHandler
{
public:
	virtual bool handleEvent(const ny::Event& event) override
	{
		ny::debug("Ayyyyy got the event");
		return true;
	}
};

class MyWindow : public ny::ToplevelWindow
{
public:
	using ny::ToplevelWindow::ToplevelWindow;
	virtual bool handleEvent(const ny::Event& event) override
	{
		//ny::debug("got event with type ", event.type());
		if(ny::ToplevelWindow::handleEvent(event)) return true;

		if(event.type() == ny::eventType::dataOffer)
		{
			auto& offer = static_cast<const ny::DataOfferEvent&>(event).offer;
			ny::debug("yooo, event offer: ", offer->types().types.size());
			for(auto type : offer->types().types)
				ny::debug("available: ", (int) type);

			offer->data(ny::dataType::text, [](const ny::DataOffer&, int format, const std::any& text) {
					ny::debug("called");
					ny::debug(&text);
					if(!text.empty()) ny::debug(std::any_cast<std::string>(text));
					else ny::debug("oooh");
				});
			return true;
		}

		return false;
	}
};

class MyEvent : public ny::SizeEvent {};

int main()
{
	ny::debug(__cplusplus);

	ny::App::Settings s;
	s.multithreaded = true;
	ny::App app(s);

	ny::WindowSettings settings;
	settings.position = {300, 300};
	settings.draw = ny::DrawType::gl;

	MyWindow window(app, ny::Vec2ui(800, 500), "ny Window Test", settings);
	window.windowContext()->show();

	window.windowContext()->addWindowHints(ny::WindowHints::acceptDrop);
	//window.windowContext()->addWindowHints(ny::WindowHints::customDecorated);
	//window.maximize();

	ny::Image icon("icon.jpg");
	window.icon(icon);

	ny::Gui myGui(window);

	ny::Button myButton(myGui, {100, 100}, {100, 45});
	myButton.label("Maximize");
	myButton.onClick = [&]{ std::cout << "Clicked!\n"; window.maximize(); };

	ny::Button myButton2(myGui, {300, 300}, {100, 45});
	myButton2.label("Fullscreen");
	myButton2.onClick = [&]{ std::cout << "Clicked!\n"; window.fullscreen(); };

	ny::Button myButton3(myGui, {0, 400}, {100, 45});
	myButton3.label("Normal");
	myButton3.onClick = [&]{ std::cout << "Clicked!\n"; window.reset(); };

	ny::Button myButton4(myGui, {500, 200}, {100, 45});
	myButton4.label("Minimize");
	myButton4.onClick = [&]{ std::cout << "Clicked!\n"; window.minimize(); };

	ny::Rectangle rect({100, 100}, {100, 100});
	MyHandler myHandler;
	MyEvent myEvent;
	myEvent.handler = &myHandler;

	window.onDraw += [&](ny::DrawContext& dc) {
		ny::debug("DRAW");
		//dc.clear(ny::Color::green);
		//dc.draw({rect, ny::Color::black});

		//myGui.draw(dc);

		static bool version = false;
		if(!version)
		{
			ny::debug("SEND");

			std::thread thread([&]{ app.dispatcher().dispatch(std::move(myEvent)); });
			thread.detach();

			auto glContext = ny::GlContext::current();
			if(!glContext) return;

			ny::log("GL Version: ", glContext->version().name());
			ny::log("GLSL Version: ", glContext->preferredGlslVersion().name());

			for(auto& glsl : glContext->glslVersions())
				ny::log(glsl.name());

			version = true;
		}
	};

	auto& ac = dynamic_cast<ny::WinapiAppContext&>(app.appContext());
	ny::log("clipboard: ", ac.clipboard());
	ny::log("evDispatcher: ", &app.dispatcher());

	ny::LoopControl control;
	return app.run(control);


/*
	ny::Timer timer;
	while(app.appContext().dispatchEvents(app.dispatcher()) && window.windowContext())
	{
		auto guard = window.windowContext()->draw();
		guard.dc().clear(ny::Color::green);
		ny::debug(timer.elapsedTime().microseconds());
		timer.reset();
	}
*/
	//

	//while(app.dispatch() == true);

/*
	ny::WinapiAppContext appContext;
	ny::GdiWinapiWindowContext wc(appContext);

	ny::EventDispatcher dispatcher;
	ny::LoopControl control;
	//appContext.dispatchLoop(dispatcher, control);

	while(true) {
		appContext.dispatchEvents(dispatcher);

		auto guard = wc.draw();
		guard.dc().clear(ny::Color::white);
	}
	*/
}
