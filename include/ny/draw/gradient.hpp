#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/color.hpp>

#include <vector>

namespace ny
{

//gradient base
class ColorGradient
{
public:
	struct Stop
	{
		float position;
		Color color;
	};

protected:
	std::vector<ColorStop> stops_;

public:
	void addPoint(const Stop& p);
	void addPoint(float position, const Color& col);

	Color colorAt(float position) const;
	const std::vector<Stops> stops() const { return stops_; };
};

}
