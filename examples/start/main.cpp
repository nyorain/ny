#include <iostream>
#include <ny/ny.hpp>
#include <ny/draw/font.hpp>
#include <ny/window/toplevel.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/draw/gl/glad/glad.h>
#include <ny/draw/gl/drawContext.hpp>
#include <ny/draw/gl/shaderGenerator.hpp>

constexpr const char* colorFS = 
R"SRC(
uniform vec4 fColor;

void main()
{
    %fragColor = fColor; 
}
)SRC";

int main()
{
	ny::FragmentShaderGenerator generator;
	generator.code(colorFS);

	std::cout << generator.generate({ny::GlContext::Api::openGLES, 1, 0}) << "\n";
	std::cout << generator.generate({ny::GlContext::Api::openGLES, 3, 1}) << "\n";
	std::cout << generator.generate({ny::GlContext::Api::openGL, 2, 0}) << "\n";
	std::cout << generator.generate({ny::GlContext::Api::openGL, 4, 5}) << "\n";

	return 1;

	ny::App app;

	ny::WindowSettings settings;
	//settings.glPref = ny::Preference::must;
	ny::ToplevelWindow window(ny::vec2ui(800, 500), "test", settings);
	window.maximizeHint(0);

	auto rct = ny::Rectangle({0.f, 0.f}, {100.f, 100.f});

	ny::Font font("Ubuntu-M.ttf");
	auto t = ny::Text({100.f, 100.f}, "Hello World", 60);
	t.rotate(-45);
	t.font(font);

	window.onDraw = [&](ny::DrawContext& dc)
		{ 
	//		dc.clear(ny::Color(200, 150, 230));

	//		dc.mask(rct);
	//		dc.fill(ny::Color::white);

	//		dc.mask(t);
	//		dc.fill(ny::Color::white);
		};

	window.onKey = [](const ny::KeyEvent& k){ std::cout << "key: " << k.text << "\n"; };
	window.show();

	return app.mainLoop();
}
