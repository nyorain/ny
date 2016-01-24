#include <iostream>
#include <ny/ny.hpp>
#include <ny/window/toplevel.hpp>
#include <ny/app/keyboard.hpp>

int main()
{
	ny::App app;

	ny::WindowSettings settings;
	settings.glPref = ny::Preference::Must;
	ny::ToplevelWindow window(ny::vec2ui(800, 500), "test", settings);
	window.onDraw = [](ny::DrawContext& dc){ dc.clear(ny::Color::red); };
	window.onKey = [](const ny::KeyEvent& k){ std::cout << "key: " << k.text << "\n"; };
	window.show();

	return app.mainLoop();
}
