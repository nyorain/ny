#include <ny/draw/pen.hpp>

namespace ny
{

Pen::Pen(const Brush& brush, float width, const DashStyle& style)
	: width_(width), dashStyle_(style), brush_(brush)
{
}

}
