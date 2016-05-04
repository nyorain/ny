#include <iostream>
#include <ny/ny.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/gdi.hpp>

int main()
{
	ny::App::Settings s;
	s.multithreaded = false;
	ny::App app(s);

	ny::WindowSettings settings;
	settings.position = {300, 300};

	ny::ToplevelWindow window(app, ny::Vec2ui(800, 500), "ny Window Test", settings);
	window.windowContext()->show();

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
	//window.onDraw += [](ny::DrawContext& dc) {
	//	ny::sendDebug("DRAW"); dc.clear(ny::Color::white); };

	ny::LoopControl control;
	return app.run(control);
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
