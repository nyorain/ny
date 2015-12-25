#include <ny/draw/shape.hpp>
#include <ny/draw/font.hpp>
#include <nytl/log.hpp>

namespace ny
{

//PathSegment
PathSegment::PathSegment(const vec2f& position, PathSegment::Type t) noexcept
 : type_(t), position_(position)
{
    switch(type_)
    {
        case Type::cubicCurve:
            controlPoint_.~vec();
            cubicCurveControlPoints_ = {{0, 0}, {0, 0}};
            break;

        case Type::arc:
            controlPoint_.~vec();
            arc_ = ArcData{};
            break;

        default: break;
    }
}

PathSegment::~PathSegment() noexcept //=default? really needed here?
{
    resetUnion();
}

PathSegment::PathSegment(const PathSegment& other) noexcept
    : type_(other.type_), position_(other.position_)
{
    switch(type_)
    {
        case Type::cubicCurve:
            controlPoint_.~vec();
            cubicCurveControlPoints_ = other.cubicCurveControlPoints_;
            break;

        case Type::arc:
            controlPoint_.~vec();
            arc_ = other.arc_;
            break;

        default:
            controlPoint_ = other.controlPoint_;
            break;
    }
}

PathSegment& PathSegment::operator=(const PathSegment& other) noexcept
{
    resetUnion();

    type_ = other.type_;
    position_ = other.position_;

    switch(type_)
    {
        case Type::cubicCurve: cubicCurveControlPoints_ = other.cubicCurveControlPoints_; break;
        case Type::arc: arc_ = other.arc_; break;
        default: controlPoint_ = other.controlPoint_; break;
    }

    return *this;
}

void PathSegment::resetUnion()
{
    switch(type_)
    {
        case Type::cubicCurve: cubicCurveControlPoints_.~pair(); break;
        case Type::arc: arc_.~ArcData(); break;
        default: controlPoint_.~vec(); break;
    }
}

//
void PathSegment::line()
{
    resetUnion();

    type_ = Type::line;
    controlPoint_ = {};
}

void PathSegment::smoothQuadCurve()
{
    resetUnion();

    type_ = Type::smoothQuadCurve;
    controlPoint_ = {};
}

void PathSegment::quadCurve(const vec2f& control)
{
    resetUnion();

    type_ = Type::quadCurve;
    controlPoint_ = control;
}

void PathSegment::smoothCubicCurve(const vec2f& control)
{
    resetUnion();

    type_ = Type::smoothCubicCurve;
    controlPoint_ = control;
}

void PathSegment::cubicCurve(const vec2f& control1, const vec2f& control2)
{
    resetUnion();

    type_ = Type::cubicCurve;
    cubicCurveControlPoints_ = {control1, control2};
}

void PathSegment::arc(const ArcData& data)
{
    resetUnion();

    type_ = Type::arc;
    arc_ = data;
}

vec2f PathSegment::controlPoint() const
{
    if(type_ == Type::quadCurve || type_ == Type::smoothCubicCurve)
    {
        return controlPoint_;
    }

    sendWarning("ny::PathSegment::controlPoint: invalid type");
    return {};
}

std::pair<vec2f, vec2f> PathSegment::cubicCurveControlPoints() const
{
    if(type_ == Type::cubicCurve)
    {
        return cubicCurveControlPoints_;
    }

    sendWarning("ny::PathSegment::cubicCurveControlPoints: invalid type");
    return {{0.f, 0.f}, {0.f, 0.f}};
}

ArcData PathSegment::arcData() const
{
    if(type_ == Type::arc)
    {
        return arc_;
    }

    sendWarning("ny::PathSegment::arcData: invalid type");
    return ArcData{};
}


//PlainSubpath
PlainSubpath::PlainSubpath(const std::vector<vec2f>& points, bool closed)
    : std::vector<vec2f>(points), closed_(closed)
{
}

//Subpath
Subpath::Subpath(const vec2f& start) : start_(start)
{
}

const PathSegment& Subpath::line(const vec2f& position)
{
    segments_.emplace_back(position, PathSegment::Type::line);
    return segments_.back();
}

const PathSegment& Subpath::smoothQuadCurve(const vec2f& position)
{
    segments_.emplace_back(position, PathSegment::Type::smoothQuadCurve);
    return segments_.back();
}

const PathSegment& Subpath::quadCurve(const vec2f& position, const vec2f& control)
{
    segments_.emplace_back(position, PathSegment::Type::quadCurve);
    segments_.back().quadCurve(control);
    return segments_.back();
}

const PathSegment& Subpath::smoothCubicCurve(const vec2f& position, const vec2f& control)
{
    segments_.emplace_back(position, PathSegment::Type::smoothCubicCurve);
    segments_.back().smoothCubicCurve(control);
    return segments_.back();
}

const PathSegment& Subpath::cubicCurve(const vec2f& pos, const vec2f& con1, const vec2f& con2)
{
    segments_.emplace_back(pos, PathSegment::Type::cubicCurve);
    segments_.back().cubicCurve(con1, con2);
    return segments_.back();
}

const PathSegment& Subpath::arc(const vec2f& pos, const ArcData& data)
{
    segments_.emplace_back(pos, PathSegment::Type::arc);
    segments_.back().arc(data);
    return segments_.back();
}

const vec2f& Subpath::currentPosition() const
{
    if(segments_.empty()) return start_;
    return segments_.back().position();
}

PlainSubpath Subpath::bake() const
{
    //TODO
    PlainSubpath ret;
	ret.push_back(startPoint());

    for(auto& seg : segments_)
    {
        ret.push_back(seg.position());
    }

	if(closed())
		ret.close();

    return ret;
}


//Path
Path::Path(const vec2f& start)
{
    subpaths_.emplace_back(start);
}

Path::Path(const Subpath& sub) : subpaths_{sub}
{
}

Subpath& Path::newSubpath()
{
    subpaths_.emplace_back(currentPosition());
    return subpaths_.back();
}

Subpath& Path::move(const vec2f& position)
{
    //rlly check this case? is this behaviour expected? -> interface
    if(currentSubpath().segments().empty())
    {
        currentSubpath().startPoint(position);
        return currentSubpath();
    }

    subpaths_.emplace_back(position);
    return subpaths_.back();
}

const PathSegment& Path::line(const vec2f& position)
{
    return currentSubpath().line(position);
}
const PathSegment& Path::Path::smoothQuadCurve(const vec2f& position)
{
    return currentSubpath().smoothQuadCurve(position);
}
const PathSegment& Path::quadCurve(const vec2f& position, const vec2f& control)
{
    return currentSubpath().quadCurve(position, control);
}
const PathSegment& Path::smoothCubicCurve(const vec2f& position, const vec2f& control)
{
    return currentSubpath().smoothCubicCurve(position, control);
}
const PathSegment& Path::cubicCurve(const vec2f& pos, const vec2f& con1, const vec2f& con2)
{
    return currentSubpath().cubicCurve(pos, con1, con2);
}
const PathSegment& Path::arc(const vec2f& position, const ArcData& data)
{
    return currentSubpath().arc(position, data);
}

Subpath& Path::close()
{
    currentSubpath().close();
    return newSubpath();
}

const vec2f& Path::currentPosition() const
{
    return currentSubpath().currentPosition();
}

//rectangle baking
Path Rectangle::asPath() const
{
	//todo: borderradius values.
	//
	//auto testVec = lessThanEqual(borderRadius_, 0);
	//nytl::sendLog("radius: ", borderRadius_);
	//nytl::sendLog("testvec: ", testVec);

    if(1) //all(testVec)
    {
        Path p({0.f, 0.f});
        p.line(vec2f(size_.x, 0));
        p.line(size_);
        p.line(vec2f(0, size_.y));
        p.close();

        p.copyTransform(*this);
        return p;
    }
    else
    {
        rect2f me;
        me.size = size_; //dont copy position since it will be copied with copyTransform()

        Path p(me.topLeft() + vec2f(0, borderRadius_[0]));
        p.arc(me.topLeft() + vec2f(borderRadius_[0], 0),
            {vec2f(borderRadius_[0], borderRadius_[0]), 0, 1});

        p.line(me.topRight() - vec2f(borderRadius_[1], 0));
        p.arc(me.topRight() + vec2f(0, borderRadius_[1]),
            {vec2f(borderRadius_[1], borderRadius_[1]), 0, 1});

        p.line(me.bottomRight() - vec2f(0, borderRadius_[2]));
        p.arc(me.bottomRight() - vec2f(borderRadius_[2], 0),
            {vec2f(borderRadius_[2], borderRadius_[2]), 0, 1});

        p.line(me.bottomLeft() + vec2f(borderRadius_[3], 0));
        p.arc(me.bottomLeft() - vec2f(0, borderRadius_[3]),
            {vec2f(borderRadius_[3], borderRadius_[3]), 0, 1});

        p.close();
        p.copyTransform(*this);
        return p;
    }
}


//Text
Text::Text(const std::string& s, float size)
    : size_(size), string_(s), font_(&Font::defaultFont())
{
}

Text::Text(const vec2f& position, const std::string& s, float size)
    : transformable2(position), size_(size), string_(s), font_(&Font::defaultFont())
{
}

//Circle
Path Circle::asPath() const
{
    //TODO: better solution should be possible; kinda hacky here; 2 arcs, not correctly closed

    Path p(vec2f(-radius_, 0)); //top point

    p.arc(vec2f(radius_, 0), {vec2f(radius_, radius_), 0, 1}); //bottom point
    p.arc(vec2f(-radius_, 0), {vec2f(radius_, radius_), 0, 1}); //top point

    p.close();
    p.copyTransform(*this);
    return p;
}

//PathBase
PathBase::~PathBase()
{
	resetUnion();
}

PathBase::PathBase(const PathBase& other)
	: type_(other.type_), circle_()
{
	resetUnion();
	switch(type_)
	{
		case Type::text: text_ = other.text_; break;
		case Type::rectangle: rectangle_ = other.rectangle_; break;
		case Type::circle: circle_ = other.circle_; break;
		case Type::path: path_ = other.path_; break;
	}
}

PathBase& PathBase::operator=(const PathBase& other)
{
	resetUnion();
	type_ = other.type_;

	switch(type_)
	{
		case Type::text: text_ = other.text_; break;
		case Type::rectangle: rectangle_ = other.rectangle_; break;
		case Type::circle: circle_ = other.circle_; break;
		case Type::path: path_ = other.path_; break;
	}

	return *this;
}

PathBase::PathBase(PathBase&& other) noexcept
	: type_(other.type_), circle_()
{
	resetUnion();
	switch(type_)
	{
		case Type::text: text_ = std::move(other.text_); break;
		case Type::rectangle: rectangle_ = std::move(other.rectangle_); break;
		case Type::circle: circle_ = std::move(other.circle_); break;
		case Type::path: path_ = std::move(other.path_); break;
	}

	other.type_ = Type::circle;
	other.circle_ = Circle();
}

PathBase& PathBase::operator=(PathBase&& other) noexcept
{
	resetUnion();
	type_ = other.type_;

	switch(type_)
	{
		case Type::text: text_ = std::move(other.text_); break;
		case Type::rectangle: rectangle_ = std::move(other.rectangle_); break;
		case Type::circle: circle_ = std::move(other.circle_); break;
		case Type::path: path_ = std::move(other.path_); break;
	}

	other.type_ = Type::circle;
	other.circle_ = Circle();

	return *this;
}

void PathBase::resetUnion()
{
	switch(type_)
	{
		case Type::text: text_.~Text(); break;
		case Type::rectangle: rectangle_.~Rectangle(); break;
		case Type::circle: circle_.~Circle(); break;
		case Type::path: path_.~Path(); break;
	}
}

void PathBase::text(const Text& obj)
{
	resetUnion();
	type_ = Type::text;
	text_ = obj;
}

void PathBase::rectangle(const Rectangle& obj)
{
	resetUnion();
	type_ = Type::rectangle;
	rectangle_ = obj;
}

void PathBase::circle(const Circle& obj)
{
	resetUnion();
	type_ = Type::circle;
	circle_ = obj;
}

void PathBase::path(const Path& obj)
{
	resetUnion();
	type_ = Type::path;
	path_ = obj;
}

void PathBase::path(Path&& obj)
{
	resetUnion();
	path_ = std::move(obj);
}

const transformable2& PathBase::transformable() const
{
	switch(type_)
	{
		case Type::text: return text_;
		case Type::rectangle: return rectangle_;
		case Type::circle: return circle_; 
		case Type::path: return path_;
	}
}

transformable2& PathBase::transformable() 
{
	switch(type_)
	{
		case Type::text: return text_;
		case Type::rectangle: return rectangle_;
		case Type::circle: return circle_; 
		case Type::path: return path_;
	}
}

}
