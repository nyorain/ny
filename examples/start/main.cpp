#include <iostream>
#include <ny/ny.hpp>

int main()
{
	ny::App::Settings s;
	//s.multithreaded = false;
	ny::App app(s);

	ny::ToplevelWindow window(app, ny::Vec2ui(800, 500), "ny Window Test");

	//window.fullscreen();

	//ny::Image icon("icon.jpg");
	//window.icon(icon);

	//ny::Gui myGui(window);
	//ny::Button myButton(myGui, {650, 400}, {100, 45});
	//myButton.label("Close");

	//myButton.onClick = [&]{ std::cout << "Clicked!\n"; window.close(); };

	window.onDraw = [](ny::DrawContext& dc) { dc.clear(ny::Color::white); };

	//window.show();

	ny::LoopControl control;
	return app.run(control);

	//while(app.dispatch() == true);
}
