#pragma once

#include <ny/include.hpp>
#include <nytl/vec.hpp>
#include <nytl/refVec.hpp>

#include <string>
#include <cstdint>

namespace ny
{

///Represents a rgba-color in unsigned char range (0-255).
class Color
{
public:
	using Value = std::uint8_t;

public:
	Value r = 0;
	Value g = 0;
	Value b = 0;
	Value a = 0;

public:
	constexpr Color() = default;
	~Color() = default;
    constexpr Color(Value rx, Value gx, Value bx, Value ax = 255) noexcept
		: r(rx), g(gx), b(bx), a(ax) {}

	constexpr Color(const Vec3uc& val) noexcept : r(val.x), g(val.y), b(val.z), a(255) {}
	constexpr Color(const Vec4uc& val) noexcept : r(val.x), g(val.y), b(val.z), a(val.w) {}

	///Create the Color object from a packaged rgba-color int.
	///Can be used to create it from a hexadecimal number e.g. Color(0xCC55FFFF)
	Color(std::uint32_t color) noexcept;

	//TODO
	//Create the Color object from an string (color name or hexadecimal [e.g. #ccca or #ffffff]).
	//Color(const std::string& color);

	std::uint32_t& asInt();

	std::uint32_t rgbaInt() const;
	std::uint32_t argbInt() const;

	void rgbaInt(std::uint32_t color);
	void argbInt(std::uint32_t color);

    void normalized(float& pr, float& pg, float& pb, float& pa) const;
    void normalized(float& pr, float& pg, float& pb) const;

    RefVec4uc rgba() { return {r ,g, b, a}; }
    RefVec3uc rgb() { return {r, g, b}; }

    Vec4uc rgba() const { return {r ,g, b, a}; }
    Vec3uc rgb() const { return {r, g, b}; }

    Vec4f rgbaNorm() const { return Vec4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f); }
    Vec3f rgbNorm() const { return Vec3f(r / 255.f, g / 255.f, b / 255.f); }

public:
    const static Color red;
    const static Color green;
    const static Color blue;
    const static Color white;
    const static Color black;
    const static Color none;
};

}
