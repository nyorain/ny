#include <ny/graphics/shape.hpp>


namespace ny
{


std::vector<vec2f> bakePoints(const std::vector<point>& vec)
{
    std::vector<vec2f> ret;

    for(unsigned int i(0); i < vec.size(); i++)
    {
        ret.push_back(vec[i].position);
    }

    return std::move(ret);
}



//customPaths
customPath::customPath(vec2f start)
{
    points_.push_back(start);
    needBake_ = 1;
}

void customPath::addLine(vec2f p)
{
    points_.push_back(point(p));
    points_.back().setLinearDraw();
    needBake_ = 1;
}
void customPath::addLine(float x, float y)
{
    points_.push_back(point(x,y));
    points_.back().setLinearDraw();
    needBake_ = 1;
}

void customPath::addBezier(vec2f p, const bezierData& d)
{
    points_.push_back(point(p));
    points_.back().setBezierDraw(d);
    needBake_ = 1;
}
void customPath::addBezier(vec2f p,vec2f a, vec2f b)
{
    points_.push_back(point(p));
    points_.back().setBezierDraw(a, b);
    needBake_ = 1;
}

void customPath::addArc(vec2f p, const arcData& d)
{
    points_.push_back(point(p));
    points_.back().setArcDraw(d);
    needBake_ = 1;
}
void customPath::addArc(vec2f p, float radius, arcType type)
{
    points_.push_back(point(p));
    points_.back().setArcDraw(radius, type);
    needBake_ = 1;
}

void customPath::bake(int precision) const
{

}

}
