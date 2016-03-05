#pragma once

#include <ny/include.hpp>
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
	std::vector<Stop> stops_;

public:
	void addStop(const Stop& p);
	void addStop(float position, const Color& col);

	Color colorAt(float position) const;
	const std::vector<Stop>& stops() const { return stops_; };
};

}
