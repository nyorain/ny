#pragma once

#include <ny/include.hpp>
#include <ny/draw/color.hpp>
#include <ny/draw/gradient.hpp>

#include <nytl/vec.hpp>
#include <nytl/line.hpp>
#include <nytl/rect.hpp>

namespace ny
{

//ImageBrush
struct TextureBrush
{
	const Texture* texture;
	Rect2f extents;
};

//LinearGradientBrush
struct LinearGradientBrush
{
	ColorGradient gradient;
	line2f line;
};

//RadialGradientBrush
struct RadialGradientBrush
{
	ColorGradient gradient;
	Vec2f center;
	float radius;
};

///The Brush class represents a pattern to fill paths or entire surfaces.
///The Brush class is basically just an union which can hold a Color, an ImageBrush, a
///linearGradientBrush or a RadialGradienrtBrush.
class Brush
{
public:
	enum class Type
	{
		color,
		linearGradient,
		radialGradient,
		texture
	};

	static const Brush none;

protected:
	Type type_;
	union
	{
		Color color_;
		LinearGradientBrush linearGradient_;
		RadialGradientBrush radialGradient_;
		TextureBrush texture_;
	};

	///Destructs the current active union member.
	void resetUnion();

public:
	Brush(const Color& c = Color::none);
	Brush(const LinearGradientBrush& grad);
	Brush(const RadialGradientBrush& grad);
	Brush(const TextureBrush& tex);
	~Brush();

	//no move operators since they do not differ
	Brush(const Brush& other);
	Brush& operator=(const Brush& other);

	//
	Type type() const { return type_; }

	const Color& color() const { return color_; }
	LinearGradientBrush linearGradient() const { return linearGradient_; }
	RadialGradientBrush radialGradient() const { return radialGradient_; }
	TextureBrush textureBrush() const { return texture_; }

	void color(const Color& col);
	void linearGradientBrush(const LinearGradientBrush& grad);
	void radialGradientBrush(const RadialGradientBrush& grad);
	void textureBrush(const TextureBrush& tex);
};

}
