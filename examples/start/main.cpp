#include <iostream>
#include <ny/ny.hpp>

int main()
{
	ny::App app;
	
	ny::Frame frame(app, "Just a test", ny::vec2ui(800, 500));
	ny::Gui gui(frame);

	ny::Button button(gui, "Click me!");
	button.onClick([]{ std::cout << "Clicked\n"; });

	ny::Textfield textfield(gui);
	textfield.onEnter([](ny::Textfield& tf){ std::cout << "Entered: " << tf.label() << "\n"; });

	return app.run();
}
