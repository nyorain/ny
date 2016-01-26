#include <ny/draw/pen.hpp>

namespace ny
{

const Pen Pen::none {};

Pen::Pen(const Brush& brush, float width, const DashStyle& style)
	: width_(width), dashStyle_(style), brush_(brush)
{
}

}
