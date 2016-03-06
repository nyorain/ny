#include <iostream>
#include <ny/ny.hpp>

int main()
{
	ny::App app {};

	ny::WindowSettings settings;
	ny::ToplevelWindow window(app, ny::Vec2ui(800, 500), "test", settings);

	//ny::Rectangle rct({100, 100}, {100, 100});

	//ny::Gui myGui(window);
	//ny::Button myButton(myGui, {650, 400}, {100, 45});
	//myButton.label("Close");

	//myButton.onClick = [&]{ std::cout << "Clicked!\n"; window.close(); };

	window.show();

	//ny::LoopControl control;
	//return app.run(control);

	while(app.dispatch() == true);
}
