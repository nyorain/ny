#include <ny/shape.hpp>


namespace ny
{


std::vector<vec2f> bakePoints(const std::vector<point>& vec)
{
    std::vector<vec2f> ret;

    for(unsigned int i(0); i < vec.size(); i++)
    {
        ret.push_back(vec[i].position);
    }

    return ret;
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

///////////////////////////////////////////////////////7
path::path(const path& other) : type_(other.type_)
{
    switch(type_)
    {
        case pathType::text: text_ = other.text_; break;
        case pathType::rectangle: rectangle_ = other.rectangle_; break;
        case pathType::circle: circle_ = other.circle_; break;
        case pathType::custom: custom_ = other.custom_; break;
    }
}

path::~path()
{
    switch (type_)
    {
        case pathType::text: text_.~text(); break;
        case pathType::rectangle: rectangle_.~rectangle(); break;
        case pathType::circle: circle_.~circle(); break;
        case pathType::custom: custom_.~customPath(); break;
    }
}

const transformable2& path::getTransformable() const
{
    switch (type_)
    {
        case pathType::text: return text_;
        case pathType::rectangle: return rectangle_;
        case pathType::circle: return circle_;
        default: return custom_;
    }
}

transformable2& path::getTransformable()
{
    switch (type_)
    {
        case pathType::text: return text_;
        case pathType::rectangle: return rectangle_;
        case pathType::circle: return circle_;
        default: return custom_;
    }
}
///////////////////////////////////////////////////////////////7
customPath rectangle::getAsCustomPath() const
{
    rect2f me = *this;

    customPath p(me.topLeft() + vec2f(0, borderRadius_[0]));
    p.addArc(me.topLeft() + vec2f(borderRadius_[0], 0), borderRadius_[0], arcType::right);

    p.addLine(me.topRight() - vec2f(borderRadius_[1], 0));
    p.addArc(me.topRight() + vec2f(0, borderRadius_[1]), borderRadius_[1], arcType::right);

    p.addLine(me.bottomRight() - vec2f(0, borderRadius_[2]));
    p.addArc(me.bottomRight() - vec2f(borderRadius_[2], 0), borderRadius_[2], arcType::right);

    p.addLine(me.bottomLeft() + vec2f(borderRadius_[3], 0));
    p.addArc(me.bottomLeft() - vec2f(0, borderRadius_[3]), borderRadius_[3], arcType::right);

    //p.close();

    return p;
}

customPath circle::getAsCustomPath() const
{
    customPath p(position_ + vec2f(radius_, 0)); //top point

    p.addArc(position_ + vec2f(radius_, radius_), radius_, arcType::right); //bottom point
    p.addArc(position_ + vec2f(radius_, 0), radius_, arcType::right); //top point

    //p.close();

    return p;
}

}
