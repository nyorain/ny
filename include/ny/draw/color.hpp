#pragma once

#include <ny/draw/include.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Represents a rgba-color in unsigned char range (0-255).
class Color
{
public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

public:
    constexpr Color(unsigned char rx = 0, unsigned char gx = 0,
			unsigned char bx = 0, unsigned char ax = 255) noexcept
		: r(rx), g(gx), b(bx), a(ax) {}

	constexpr Color(const vec3uc& comps) noexcept
		: r(comps.x), g(comps.y), b(comps.z), a(255) {}
	constexpr Color(const vec4uc& comps) noexcept
		: r(comps.x), g(comps.y), b(comps.z), a(comps.w) {}

    unsigned int asInt();
    void normalized(float& pr, float& pg, float& pb, float& pa) const;
    void normalized(float& pr, float& pg, float& pb) const;

    vec4uc rgba() const { return vec4uc(r,g,b,a); }
    vec3uc rgb() const { return vec3uc(r,g,a); }

    vec4f rgbaNorm() const { return vec4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f); }
    vec3f rgbNorm() const { return vec3f(r / 255.f, g / 255.f, b / 255.f); }

public:
    const static Color red;
    const static Color green;
    const static Color blue;
    const static Color white;
    const static Color black;
    const static Color none;
};

//multiply operator
Color operator*(float fac, const Color& c)
{
	return Color(fac * c.rgba());
}

}
