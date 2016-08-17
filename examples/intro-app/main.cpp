#include <ny/app.hpp>

int main()
{
	ny::App app; //create the app with default settings
	ny::ToplevelWindow window(app); //create the window with default settings
	app.run(); //run the main loop
}
