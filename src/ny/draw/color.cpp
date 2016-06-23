#include <ny/draw/color.hpp>

namespace ny
{

const Color Color::none{0, 0, 0, 0};
const Color Color::red{255, 0, 0};
const Color Color::green{0, 255, 0};
const Color Color::blue{0, 0, 255};
const Color Color::white{255, 255, 255};
const Color Color::black{0, 0, 0};

//TODO: endianess?

Color::Color(std::uint32_t color) noexcept
{
	auto* ptr = reinterpret_cast<Value*>(&color);
	a = *ptr;
	b = *++ptr;
	g = *++ptr;
	r = *++ptr;
}

std::uint32_t Color::rgbaInt() const
{
	std::uint32_t ret = (a << 24) | (b << 16) | (g << 8) | (r);
    return ret;
}

std::uint32_t Color::argbInt() const
{
	std::uint32_t ret = (b << 24) | (g << 16) | (r << 8) | (a);
    return ret;
}

std::uint32_t& Color::asInt()
{
	return reinterpret_cast<std::uint32_t&>(r);
}

void Color::argbInt(std::uint32_t color)
{
	auto* ptr = reinterpret_cast<Value*>(&color);
	b = *ptr;
	g = *++ptr;
	r = *++ptr;
	a = *++ptr;
}

void Color::rgbaInt(std::uint32_t color)
{
	auto* ptr = reinterpret_cast<Value*>(&color);
	a = *ptr;
	b = *++ptr;
	g = *++ptr;
	r = *++ptr;
}

void Color::normalized(float& pr, float& pg, float& pb, float& pa) const
{
    pr = r / 255.f;
    pg = g / 255.f;
    pb = b / 255.f;
    pa = a / 255.f;
}

void Color::normalized(float& pr, float& pg, float& pb) const
{
    pr = r / 255.f;
    pg = g / 255.f;
    pb = b / 255.f;
}

}
