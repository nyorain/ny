#include <iostream>
#include <ny/ny.hpp>

int main()
{
	ny::App::Settings s;
	s.multithreaded = false;
	ny::App app(s);

	ny::ToplevelWindow window(app, ny::Vec2ui(800, 500), "ny Window Test");
	window.windowContext()->show();

	//window.maximize();

	ny::Image icon("icon.jpg");
	window.icon(icon);

	ny::Gui myGui(window);
	ny::Button myButton(myGui, {100, 100}, {100, 45});
	myButton.label("Maximize");

	myButton.onClick = [&]{ std::cout << "Clicked!\n"; window.maximize(); };

	/*
	window.onDraw += [](ny::DrawContext& dc) { 
		ny::sendDebug("DRAW"); dc.clear(ny::Color::white); };
	*/

	ny::LoopControl control;
	return app.run(control);
	//

	//while(app.dispatch() == true);
}
