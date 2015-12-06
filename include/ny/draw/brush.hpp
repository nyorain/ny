#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/color.hpp>
#include <ny/draw/gradient.hpp>

namespace ny
{

//ImageBrush
struct ImageBrush
{
	Image* image;
	rect2f extents;
};

//LinearGradientBrush
struct LinearGradientBrush
{
	ColorGradient gradient;
	line2f line;
};

//RadialGradientBrush
class RadialGradientBrush
{
	ColorGradient gradient;
	vec2f center;
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
		image
	};

	static const Brush none;

protected:
	Type type_;
	union
	{
		Color color_ = Color::none;
		LinearGradientBrush linearGradient_;
		RadialGradientBrush radialGradient_;
		ImageBrush image_;
	};

public:
	Brush(const Color& c = Color::none);
	Brush(const LinearGradientBrush& grad);
	Brush(const RadialGradientBrush& grad);
	Brush(const ImageBrush& img);
	~Brush();

	Type type() const { return type_; }

	Color color() const { return color_; }
	LinearGradientBrush linearGradient() const { return linearGradient_; }
	RadialGradientBurhs radialGradient() const { return radialGradient_; }
	ImageBrush imageBrush() const { return image_; }

	void color(const Color& c);
	void linearGradientBrush(const LinearGradientBrush& grad);
	void radialGradientBrush(const RadialGradientBrush& grad);
	void ImageBrush(const ImageBrush& grad);
};

}
