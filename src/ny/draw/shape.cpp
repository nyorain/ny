#include <ny/draw/shape.hpp>
#include <ny/draw/font.hpp>
#include <ny/base/log.hpp>

namespace ny
{

namespace bake
{

auto quadBezierFunc(const Vec2f& a, const Vec2f& b, const Vec2f& c, float t)
{
	auto t1 = 1 - t;
	return t1 * (t1 * a + t * b) + t * (t1 * b + t * c);
}

auto cubicBezierFunc(const Vec2f& a, const Vec2f& b, const Vec2f& c, const Vec2f& d, float t)
{
	auto t1 = 1 - t;
	return t1 * quadBezierFunc(a, b, c, t) + t * quadBezierFunc(b, c, d, t);
}

//quadratic bezier
std::vector<Vec2f> bakeQuadBezier(const Vec2f& a, const Vec2f& b, const Vec2f& c)
{
	//segment length
	auto lAB = length(b - a);
	auto lBC = length(c - b);
	auto totalLength = lAB + lBC;

	//adaptive point count
	auto minPoints = 5;
	auto segs = totalLength / 30;

	auto count = std::ceil(std::sqrt(segs * segs * 0.6 + minPoints * minPoints));

	//points
	std::vector<Vec2f> ret(count + 1);
	ret[0] = a;
	for(auto i = 1; i < count; ++i)
		ret[i] = quadBezierFunc(a, b, c, float(count) / i);

	ret[count] = c;
	return ret;
}

//cubic bezier
std::vector<Vec2f> bakeCubicBezier(const Vec2f& a, const Vec2f& b, const Vec2f& c, const Vec2f& d)
{
	//segment length
	auto lAB = length(b - a);
	auto lBC = length(c - b);
	auto lCD = length(d - c);
	auto totalLength = lAB + lBC + lCD;

	//adaptive point count
	auto minPoints = 10;
	auto segs = totalLength / 30;

	auto count = std::ceil(std::sqrt(segs * segs * 0.6 + minPoints * minPoints));

	//points
	std::vector<Vec2f> ret(count + 1);
	ret[0] = a;
	for(auto i = 0u; i < count; ++i)
		ret[i] = cubicBezierFunc(a, b, c, d, float(count) / i);

	ret[count] = d;
	return ret;
}

//arc
std::vector<Vec2f> bakeArc(const Vec2f& a, const CenterArcParams& params, const Vec2f& b)
{
	//see https://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
	//and http://stackoverflow.com/questions/197649/how-to-calculate-center-of-an-ellipse-by-two-points-and-radius-sizes
	auto& angle1 = params.start;
	auto& angle2 = params.end;
	auto& center = params.center;
	auto& r = params.radius;
	auto delta = angle2 - angle1;

	//TODO: adaptive point count?
	constexpr const auto count = 30;

	//points
	std::vector<Vec2f> ret(count + 1);
	ret[0] = a;

	for(auto i = 1u; i < count; ++i)
	{
		auto angle = angle1 + i * (delta / count);
		ret[i].x = r.x * cos(angle) + center.x;
		ret[i].y = r.y * sin(angle) + center.y;
	}

	ret[count] = b;
	return ret;
}

//bake pathSegment curves
std::vector<Vec2f> bake(const Vec2f& old, const PathSegment& seg, Vec2f& lastControl)
{
	using PT = PathSegment::Type;
	std::vector<Vec2f> ret;

	switch(seg.type())
	{
		case PT::line:
		{
			ret.push_back(seg.position());
			lastControl = seg.position();
			break;
		}
		case PT::smoothQuadCurve:
		{
			ret = bakeQuadBezier(old, lastControl, seg.position());

			//reflect the control point on the next point
			//https://www.w3.org/TR/SVG/paths.html
			auto delta = seg.position() - lastControl;
			lastControl = seg.position() + delta;

			break;
		}
		case PT::quadCurve:
		{
			ret = bakeQuadBezier(old, seg.controlPoint(), seg.position());
			auto delta = seg.position() - seg.controlPoint();
			lastControl = seg.position() + delta;
			break;
		}
		case PT::smoothCubicCurve:
		{
			ret = bakeCubicBezier(old, lastControl, seg.controlPoint(), seg.position());
			auto delta = seg.position() - seg.controlPoint();
			lastControl = seg.position() + delta;
			break;
		}
		case PT::cubicCurve:
		{
			auto controls = seg.cubicCurveControlPoints();
			ret = bakeCubicBezier(old, controls.first, controls.second, seg.position());
			auto delta = seg.position() - controls.second;
			lastControl = seg.position() + delta;
			break;
		}
		case PT::arc:
		{
			ret = bakeArc(old, seg.centerArcParams(old), seg.position());
			break;
		}
	}

	return ret;
}

//Converts arc data between types
//implemented from https://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
EndArcParams arcParamsToEnd(const CenterArcParams& params, Vec2f* start = nullptr,
	Vec2f* end = nullptr)
{
	auto& r = params.radius;
	auto& center = params.center;
	auto delta = params.end - params.start;

	//start and end
	if(start) *start = Vec2f(r.x * cos(params.start) + center.x, r.y * sin(params.start) + center.y);
	if(end) *end = Vec2f(r.x * cos(params.end) + center.x, r.y * sin(params.end) + center.y);

	//fill
	EndArcParams ret;
	ret.radius = params.radius;
	ret.largeArc = abs(delta) > 180;
	ret.clockwise = delta < 0;
	return ret;
}

CenterArcParams arcParamsToCenter(const EndArcParams& params, const Vec2f& start, const Vec2f& end)
{
	auto& r = params.radius;
	auto p = ((start - end) / 2);
	auto innerTop = ((r.x * r.x * r.y * r.y) - (r.x * r.x * p.y * p.y) - (r.y * r.y * p.x * p.x));
	auto innerBottom = (r.x * r.x * p.y * p.y) + (r.y * r.y * p.x * p.x);
	auto inner = innerTop / innerBottom;
	auto sign = (params.largeArc != params.clockwise) ? 1 : -1;
	auto mult = Vec2f(r.x * p.y / r.y, -r.y * p.x / r.x);
	auto tc = sign * std::sqrt(inner) * mult;

	//angles
	auto vec1 = Vec2f((p.x - tc.x) / r.x, (p.y - tc.y) / r.y);
	auto vec2 = Vec2f((-p.x - tc.x) / r.x, (-p.y - tc.y) / r.y);
	auto angle1 = angle(Vec2f(1, 0), vec1);
	auto angle2 = angle(Vec2f(1, 0), vec2);

	//fill
	CenterArcParams ret;
	ret.radius = params.radius;
	ret.center = tc + Vec2f(sum(start) / 2, sum(end) / 2);
	ret.start = angle1;
	ret.end = angle2;
	return ret;
}

}

//PathSegment
PathSegment::PathSegment(const Vec2f& position, PathSegment::Type t) noexcept
	: type_(t), position_(position)
{
    switch(type_)
    {
        case Type::cubicCurve:
            controlPoint_.~Vec();
            cubicCurveControlPoints_ = {{0, 0}, {0, 0}};
            break;

        case Type::arc:
            controlPoint_.~Vec();
			arc_ = {};
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
            controlPoint_.~Vec();
            cubicCurveControlPoints_ = other.cubicCurveControlPoints_;
            break;

        case Type::arc:
            controlPoint_.~Vec();
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
        case Type::arc: arc_.~EndArcParams(); break;
        default: controlPoint_.~Vec(); break;
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

void PathSegment::quadCurve(const Vec2f& control)
{
    resetUnion();

    type_ = Type::quadCurve;
    controlPoint_ = control;
}

void PathSegment::smoothCubicCurve(const Vec2f& control)
{
    resetUnion();

    type_ = Type::smoothCubicCurve;
    controlPoint_ = control;
}

void PathSegment::cubicCurve(const Vec2f& control1, const Vec2f& control2)
{
    resetUnion();

    type_ = Type::cubicCurve;
    cubicCurveControlPoints_ = {control1, control2};
}

void PathSegment::arc(const CenterArcParams& params)
{
    resetUnion();

    type_ = Type::arc;

	Vec2f parseEnd;
    arc_ = bake::arcParamsToEnd(params, nullptr, &parseEnd);
	if(any(parseEnd != position()))
		warning("ny::PathSegment::arc: centerArcParams do not match position of the segment");
}

void PathSegment::arc(const EndArcParams& params)
{
	resetUnion();

	type_ = Type::arc;
	arc_= params;
}

const Vec2f& PathSegment::controlPoint() const
{
    if(type_ == Type::quadCurve || type_ == Type::smoothCubicCurve) return controlPoint_;
	throw std::logic_error("ny::PathSegment::controlPoint: invalid segment type");
}

const std::pair<Vec2f, Vec2f>& PathSegment::cubicCurveControlPoints() const
{
    if(type_ == Type::cubicCurve) return cubicCurveControlPoints_;
	throw std::logic_error("ny::PathSegment::cubicCurveControlPoints: invalid segment type");
}

const EndArcParams& PathSegment::endArcParams() const
{
    if(type_ == Type::arc) return arc_;
	throw std::logic_error("ny::PathSegment::endArcParams: invalid segment type");
}

CenterArcParams PathSegment::centerArcParams(const Vec2f& start) const
{
	if(type_ == Type::arc) return bake::arcParamsToCenter(arc_, start, position());
	throw std::logic_error("ny::PathSegment::endArcParams: invalid segment type");
}


//PlainSubpath
PlainSubpath::PlainSubpath(const std::vector<Vec2f>& points, bool closed)
    : std::vector<Vec2f>(points), closed_(closed)
{
}

//Subpath
Subpath::Subpath(const Vec2f& start) : start_(start)
{
}

const PathSegment& Subpath::line(const Vec2f& position)
{
    segments_.emplace_back(position, PathSegment::Type::line);
    return segments_.back();
}

const PathSegment& Subpath::smoothQuadCurve(const Vec2f& position)
{
    segments_.emplace_back(position, PathSegment::Type::smoothQuadCurve);
    return segments_.back();
}

const PathSegment& Subpath::quadCurve(const Vec2f& position, const Vec2f& control)
{
    segments_.emplace_back(position, PathSegment::Type::quadCurve);
    segments_.back().quadCurve(control);
    return segments_.back();
}

const PathSegment& Subpath::smoothCubicCurve(const Vec2f& position, const Vec2f& control)
{
    segments_.emplace_back(position, PathSegment::Type::smoothCubicCurve);
    segments_.back().smoothCubicCurve(control);
    return segments_.back();
}

const PathSegment& Subpath::cubicCurve(const Vec2f& pos, const Vec2f& con1, const Vec2f& con2)
{
    segments_.emplace_back(pos, PathSegment::Type::cubicCurve);
    segments_.back().cubicCurve(con1, con2);
    return segments_.back();
}

const PathSegment& Subpath::arc(const Vec2f& pos, const EndArcParams& params)
{
    segments_.emplace_back(pos, PathSegment::Type::arc);
    segments_.back().arc(params);
    return segments_.back();
}

const PathSegment& Subpath::arc(const Vec2f& pos, const CenterArcParams& params)
{
    segments_.emplace_back(pos, PathSegment::Type::arc);
    segments_.back().arc(params);
    return segments_.back();
}

const Vec2f& Subpath::currentPosition() const
{
    if(segments_.empty()) return start_;
    return segments_.back().position();
}

PlainSubpath Subpath::bake() const
{
    //TODO
    PlainSubpath ret;
	ret.push_back(startPoint());

	auto lastPoint = start_;
	auto lastControl = start_;

    for(auto& seg : segments_)
    {
		auto addpoints = bake::bake(lastPoint, seg, lastControl);
		ret.insert(ret.end(), addpoints.begin(), addpoints.end());
    }

	if(closed()) ret.close();

    return ret;
}


//Path
Path::Path(const Vec2f& start)
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

Subpath& Path::move(const Vec2f& position)
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

const PathSegment& Path::line(const Vec2f& position)
{
    return currentSubpath().line(position);
}
const PathSegment& Path::Path::smoothQuadCurve(const Vec2f& position)
{
    return currentSubpath().smoothQuadCurve(position);
}
const PathSegment& Path::quadCurve(const Vec2f& position, const Vec2f& control)
{
    return currentSubpath().quadCurve(position, control);
}
const PathSegment& Path::smoothCubicCurve(const Vec2f& position, const Vec2f& control)
{
    return currentSubpath().smoothCubicCurve(position, control);
}
const PathSegment& Path::cubicCurve(const Vec2f& pos, const Vec2f& con1, const Vec2f& con2)
{
    return currentSubpath().cubicCurve(pos, con1, con2);
}
const PathSegment& Path::arc(const Vec2f& position, const CenterArcParams& params)
{
    return currentSubpath().arc(position, params);
}
const PathSegment& Path::arc(const Vec2f& position, const EndArcParams& params)
{
    return currentSubpath().arc(position, params);
}

Subpath& Path::close()
{
    currentSubpath().close();
    return newSubpath();
}

const Vec2f& Path::currentPosition() const
{
    return currentSubpath().currentPosition();
}

//Rectangle baking
Path Rectangle::asPath() const
{
	//todo: borderradius values.
	//
	//auto testVec = lessThanEqual(borderRadius_, 0);
	//sendLog("radius: ", borderRadius_);
	//sendLog("testVec: ", testVec);

    if(1) //all(testVec)
    {
        Path p({0.f, 0.f});
        p.line(Vec2f(size_.x, 0));
        p.line(size_);
        p.line(Vec2f(0, size_.y));
        p.close();

        p.transformMatrix() = transformMatrix();
        return p;
    }
    else
    {
        Rect2f me = *this;

        Path p(me.topLeft() + Vec2f(0, borderRadius_[0]));
        p.arc(me.topLeft() + Vec2f(borderRadius_[0], 0),
            {Vec2f(borderRadius_[0], borderRadius_[0]), 0, 1});

        p.line(me.topRight() - Vec2f(borderRadius_[1], 0));
        p.arc(me.topRight() + Vec2f(0, borderRadius_[1]),
            {Vec2f(borderRadius_[1], borderRadius_[1]), 0, 1});

        p.line(me.bottomRight() - Vec2f(0, borderRadius_[2]));
        p.arc(me.bottomRight() - Vec2f(borderRadius_[2], 0),
            {Vec2f(borderRadius_[2], borderRadius_[2]), 0, 1});

        p.line(me.bottomLeft() + Vec2f(borderRadius_[3], 0));
        p.arc(me.bottomLeft() - Vec2f(0, borderRadius_[3]),
            {Vec2f(borderRadius_[3], borderRadius_[3]), 0, 1});

        p.close();
        p.transformMatrix() = transformMatrix();
        return p;
    }
}


//Text
Text::Text(const std::string& s, float size)
    : size_(size), string_(s), font_(&Font::defaultFont())
{
}

Text::Text(const Vec2f& position, const std::string& s, float size)
    : position_(position), size_(size), string_(s), font_(&Font::defaultFont())
{
}

//Circle
Path Circle::asPath() const
{
    //TODO: better solution should be possible; kinda hacky here; 2 arcs, not correctly closed
	//XXX: use at least 4 arcs... pfff

    Path p(Vec2f(-radius_, 0)); //top point

    p.arc(Vec2f(radius_, 0), {Vec2f(radius_, radius_), true, false}); //bottom point
    p.arc(Vec2f(-radius_, 0), {Vec2f(radius_, radius_), true, false}); //top point

    p.close();
    p.transformMatrix() = transformMatrix();
    return p;
}

//PathBase
PathBase::~PathBase()
{
	resetUnion();
}

PathBase::PathBase(const PathBase& other)
	: type_(other.type_)
{
	switch(type_)
	{
		case Type::text: new(&text_) Text(other.text_); break;
		case Type::rectangle: new(&rectangle_) Rectangle(other.rectangle_); break;
		case Type::circle: new(&circle_) Circle(other.circle_); break;
		case Type::path: new(&path_) Path(other.path_); break;
	}
}

PathBase& PathBase::operator=(const PathBase& other)
{
	resetUnion();
	type_ = other.type_;

	switch(type_)
	{
		case Type::text: new(&text_) Text(other.text_); break;
		case Type::rectangle: new(&rectangle_) Rectangle(other.rectangle_); break;
		case Type::circle: new(&circle_) Circle(other.circle_); break;
		case Type::path: new(&path_) Path(other.path_); break;
	}

	return *this;
}

PathBase::PathBase(PathBase&& other) noexcept
	: type_(other.type_)
{
	switch(type_)
	{
		case Type::text: new(&text_) Text(std::move(other.text_)); break;
		case Type::rectangle: new(&rectangle_) Rectangle(std::move(other.rectangle_)); break;
		case Type::circle: new(&circle_) Circle(std::move(other.circle_)); break;
		case Type::path: new(&path_) Path(std::move(other.path_)); break;
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
		case Type::text: new(&text_) Text(std::move(other.text_)); break;
		case Type::rectangle: new(&rectangle_) Rectangle(std::move(other.rectangle_)); break;
		case Type::circle: new(&circle_) Circle(std::move(other.circle_)); break;
		case Type::path: new(&path_) Path(std::move(other.path_)); break;
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

const ShapeBase& PathBase::shapeBase() const
{
	switch(type_)
	{
		case Type::text: return text_;
		case Type::rectangle: return rectangle_;
		case Type::circle: return circle_;
		case Type::path: return path_;
	}
}

ShapeBase& PathBase::shapeBase()
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
