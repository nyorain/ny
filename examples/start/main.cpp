#include <iostream>
#include <ny/ny.hpp>

int main()
{
	ny::App app {};

	ny::WindowSettings settings;
	settings.glPref = ny::Preference::should;
	ny::ToplevelWindow window(ny::vec2ui(800, 500), "test", settings);

	ny::Rectangle rct({100, 100}, {100, 100});

	ny::Gui myGui(window);
	ny::Button myButton(myGui, {650, 400}, {100, 45});
	myButton.label("Close");

	myButton.onClick = [&]{ std::cout << "Clicked!\n"; window.close(); };
	
	window.show();

	return app.mainLoop();
}
