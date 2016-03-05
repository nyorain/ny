#include <ny/draw/brush.hpp>

namespace ny
{

const Brush Brush::none{};

Brush::Brush(const Color& color) : type_(Type::color), color_(color)
{
}

Brush::Brush(const LinearGradientBrush& grad) : type_(Type::linearGradient), linearGradient_(grad)
{
}

Brush::Brush(const RadialGradientBrush& grad) : type_(Type::radialGradient), radialGradient_(grad)
{
}

Brush::Brush(const TextureBrush& texture) : type_(Type::texture), texture_(texture)
{
}

Brush::~Brush()
{
	resetUnion();
}

Brush::Brush(const Brush& other) : type_(other.type_), color_()
{
	resetUnion();
	switch(type_)
	{
		case Type::color: color_ = other.color_; break;
		case Type::linearGradient: linearGradient_ = other.linearGradient_; break;
		case Type::radialGradient: radialGradient_ = other.radialGradient_; break;
		case Type::texture: texture_ = other.texture_; break;
	}
}

Brush& Brush::operator=(const Brush& other)
{
	type_ = other.type_;
	resetUnion();

	switch(type_)
	{
		case Type::color: color_ = other.color_; break;
		case Type::linearGradient: linearGradient_ = other.linearGradient_; break;
		case Type::radialGradient: radialGradient_ = other.radialGradient_; break;
		case Type::texture: texture_ = other.texture_; break;
	}

	return *this;
}

void Brush::resetUnion()
{
	switch(type_)
	{
		case Type::color: color_.~Color(); break;
		case Type::linearGradient: linearGradient_.~LinearGradientBrush(); break;
		case Type::radialGradient: radialGradient_.~RadialGradientBrush(); break;
		case Type::texture: texture_.~TextureBrush(); break;
	}
}

void Brush::color(const Color& col)
{
	resetUnion();
	type_ = Type::color;
	color_ = col;
}

void Brush::linearGradientBrush(const LinearGradientBrush& grad)
{
	resetUnion();
	type_ = Type::linearGradient;
	linearGradient_ = grad;
}
void Brush::radialGradientBrush(const RadialGradientBrush& grad)
{
	resetUnion();
	type_ = Type::radialGradient;
	radialGradient_ = grad;
}

void Brush::textureBrush(const TextureBrush& texture)
{
	resetUnion();
	type_ = Type::texture;
	texture_ = texture;
}

}
