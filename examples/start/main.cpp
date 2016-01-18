#include <iostream>
#include <ny/ny.hpp>
#include <ny/window/toplevel.hpp>

int main()
{
	ny::App app;

	ny::WindowContextSettings settings;
	//settings.glPref = ny::preference::Must;
	ny::ToplevelWindow window(ny::vec2ui(800, 500), "test", settings);
	window.onDraw([](ny::DrawContext& dc){ dc.clear(ny::Color::red); });
	window.show();

	return app.mainLoop();
}
