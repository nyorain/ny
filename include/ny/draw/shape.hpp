#pragma once

#include <ny/include.hpp>
#include <ny/draw/brush.hpp>
#include <ny/draw/pen.hpp>

#include <nytl/transform.hpp>
#include <nytl/vec.hpp>
#include <nytl/cache.hpp>
#include <nytl/rect.hpp>

#include <utility>
#include <vector>
#include <string>

namespace ny
{

///Utility base class for all shapes that allows drawing backends to store their own
///generated data for the shapes directly inside them (using the nytl::MultiCache template).
class ShapeBase : public Transform2, public MultiCache<std::string> {};

///Represents the data of an arc-type path segment in endpoint paramertization.
struct EndArcParams
{
	///The radius values of the arc (ellipse). Must be > 0.
	Vec2f radius;

	///Defines if the arc should be >180 degrees.
	bool largeArc = false;

	///Defines if the arc should be clockwise
    bool clockwise = false;
};

///Represents the data of an arc-type path segment in centerpoint paramertization.
struct CenterArcParams
{
	///The center of the arc.
	Vec2f center;

	///The radius values of the arc (ellipse). Must be > 0.
	Vec2f radius;

	///The start angle in radians. Must be > 0 and < 2pi.
	float start = 0;

	///The end angle in radians. Must be > 0 and < 2pi.
	float end = 0;
};

///The PathSegment class represents a part of a Path object, it is able to represent all
///svg path subcommands. It does always hold the position of the next point in the path and
///additionally some data that are specific to the used draw type, e.g. some kind of curve or line.
class PathSegment
{
public:
	///This enumeration holds all possible segment types. They are able to represent the svg path
	///operations. https://developer.mozilla.org/en-US/docs/Web/SVG/Tutorial/Paths holds a good
	///explanation of the different svg path segment types.
    enum class Type
    {
        line,
        smoothQuadCurve,
        quadCurve,
        smoothCubicCurve,
        cubicCurve,
        arc
    };

protected:
	Type type_ {Type::line};
    Vec2f position_ {0, 0};

	union
	{
        Vec2f controlPoint_ {0.f, 0.f}; //default activated
		std::pair<Vec2f, Vec2f> cubicCurveControlPoints_;
		EndArcParams arc_; //TODO: cache heavily computed center form for drawing
	};

	///Calls the constructor of the current active type.
	///Should be called on destruction of this object and before constructing a new member
	///of the union.
	void resetUnion();

public:
	///Constructs the segment with a given position it goes to and a given type which is defaulted
	///to line.
    PathSegment(const Vec2f& position, Type t = Type::line) noexcept;
	~PathSegment() noexcept;

	PathSegment(const PathSegment& other) noexcept;
	PathSegment& operator=(const PathSegment& other) noexcept;
	//move operations not implemented since they are the same as copy operations

	///Returns the position this segment goes to
    const Vec2f& position() const { return position_; }

	///Sets the position this segment goes to.
    void position(const Vec2f& pos){ position_ = pos; }

	///Returns the type of this Segment. See Segment::Type for information.
    Type type() const { return type_; }

	///Sets the type of this segment to line. This type requires no additional parameters.
	///Can be additionally used to set the position.
	void line();
    void line(const Vec2f& pos) { position(pos); line(); }

	///Sets the type of this segment to smoothQuadCurve. This Requires no paremeters.
	void smoothQuadCurve();
    void smoothQuadCurve(const Vec2f& pos) { position(pos); smoothQuadCurve(); }

	///Sets the type of this segment to quadCurve with the given control point.
	void quadCurve(const Vec2f& control);
    void quadCurve(const Vec2f& control, const Vec2f& pos) { position(pos); quadCurve(control); }

	///Sets the type of this segment to smoothCubicCurve with the given control point.
    void smoothCubicCurve(const Vec2f& control);
    void smoothCubicCurve(const Vec2f& control, const Vec2f& pos)
		{ position(pos); smoothCubicCurve(control); }

	///Sets the type of this segment to cubicCurve with the 2 given control points.
    void cubicCurve(const Vec2f& control1, const Vec2f& control2);
    void cubicCurve(const Vec2f& control1, const Vec2f& control2, const Vec2f& pos)
		{ position(pos); cubicCurve(control1, control2); }

	///Sets the type of this segment to arc and constructs it with the given parameters.
	///There are two types of parameter sets that can be used to construct arcs.
	///The type this function uses are end point parameters (as e.g. used by the svg spec).
	///This means that the arc is located between the previous point in the path
	///and the position of this segment, radius, and direction flags specified in the params.
	///This class does also provide a function to contruct the arc using center params,
	///although the function should be avoided (since the given params may be invalid).
	///\sa EndArcParams
    void arc(const EndArcParams& params);
    void arc(const EndArcParams& params, const Vec2f& pos) { position(pos); arc(params); }

	///Sets the type of this segment to arc and constructs it with the given parameters in center
	///form. This means that the position member of this segment represents the arc center,
	///the params specify radius and the arc size (as an angle).
	///Some graphic libraries (like e.g. cairo) use the centerpoint paramter form for
	///arcs.
	///\warning The arc should usually not be contructed using CenterArcParams since
	///the they may be invalid if they dont match the given point positions.
	///One should rather use parameters in endpoint form to construct the arc.
	///\sa CenterArcParams
	void arc(const CenterArcParams& params);
	void arc(const CenterArcParams& params, const Vec2f& pos) { position(pos); arc(params); }

	///Returns the control point this segment holds in its description. This will only return a
	///valid value if the type of this segment holds one control point in its description i.e.
	///if its quadCurve or smoothCubicCurve. Will throw a std::logic_error a warning otherwise.
    const Vec2f& controlPoint() const;

	///Returns the two control points this segment holds in its description.
	///Will throw a std::logic_error if the type of this segment is not cubicCurve.
	///TODO: Vec2<Vec2f> better as return type here?
    const std::pair<Vec2f, Vec2f>& cubicCurveControlPoints() const;

	///Will return the arc params object this segment has stored in Endpoint form.
	///Will throw a std::logic_error if the type of this segment is not arc.
    const EndArcParams& endArcParams() const;

	///Will return the ArcData object this segment has stored in center form.
	///Needs the given parameter indicating the arcs start since this information is not stored
	///inside the segment and the internal representation may have to be converted.
	///Will throw a std::logic_error if the type of this segment is not arc.
	CenterArcParams centerArcParams(const Vec2f& start) const;
};

///The PlainSubpath class represents a closed path that consists of straight lines only.
///It can be created (baked) from every normal subpath but just approximates the curves.
///Since it is basically just an array of points, the class is derived from std::vector<Vec2f>.
class PlainSubpath : public std::vector<Vec2f>
{
protected:
    bool closed_ = 0;

public:
	PlainSubpath() = default;
    PlainSubpath(const std::vector<Vec2f>& points, bool closed = 0);

	///Returns if the subpath is closed.
    bool closed() const { return closed_; }

	///Closes the subpath.
    void close(){ closed_ = 1; }
};

///The Subpath class represents a single, potentially closed, shape.
///It consists of many PathSegments, a start point and a closed flag.
class Subpath
{
protected:
    Vec2f start_;
    std::vector<PathSegment> segments_;
    bool closed_ = 0;

public:
	///Constructs the subpath with a given Startpoint
    Subpath(const Vec2f& start = {0.f, 0.f});

    const PathSegment& line(const Vec2f& position);
	const PathSegment& smoothQuadCurve(const Vec2f& position);
    const PathSegment& quadCurve(const Vec2f& position, const Vec2f& control);
    const PathSegment& smoothCubicCurve(const Vec2f& position, const Vec2f& control);
    const PathSegment& cubicCurve(const Vec2f& pos, const Vec2f& con1, const Vec2f& con2);
    const PathSegment& arc(const Vec2f& position, const EndArcParams& params);
    const PathSegment& arc(const Vec2f& position, const CenterArcParams& params);

	///Returns the current Point the Subpath stays at.
	const Vec2f& currentPosition() const;

	///Closes the Subpath.
	void close(){ closed_ = 1; }

	///Returns whether the Subpath is closed.
    bool closed() const { return closed_; }

	///Returns the segments of this Subpath.
	const std::vector<PathSegment>& segments() const { return segments_; }

	///Sets the startPoint for this Subpath.
	void startPoint(const Vec2f& start){ start_ = start; }

	///Returns the current startPoint of this Subpath.
	const Vec2f& startPoint() const { return start_; }

	///Approximiates the curves of the Subpath to create a PlainSubpath just consisting of
	///linear segments (basically a vertex array).
	///TODO: some kind of precision argument
	///TODO: caching?
    PlainSubpath bake() const;
};

///The Path class holds a vector of subpaths and can be transformed.
///It represents the svg <path> node.
class Path : public ShapeBase
{
protected:
    std::vector<Subpath> subpaths_;

public:
    Path(const Vec2f& start = {0.f, 0.f});
	Path(const Subpath& sub);
    ~Path() = default;

	///Starts a new Subpath at the current position and returns it.
    Subpath& newSubpath();

	///Starts a new Subpath at the given point. Returns the new (current after this call) Subpath.
	///If the current subpath has no segments (is empty), no new subpath will be created, the
	///current one will be used (with a changed start point) instead.
    Subpath& move(const Vec2f& point);

	const PathSegment& line(const Vec2f& position);
	const PathSegment& smoothQuadCurve(const Vec2f& position);
    const PathSegment& quadCurve(const Vec2f& position, const Vec2f& control);
    const PathSegment& smoothCubicCurve(const Vec2f& position, const Vec2f& control);
    const PathSegment& cubicCurve(const Vec2f& pos, const Vec2f& con1, const Vec2f& con2);
    const PathSegment& arc(const Vec2f& position, const EndArcParams& params);
    const PathSegment& arc(const Vec2f& position, const CenterArcParams& params);

	///Closes the current Subpath and start a new one. Returns the new Subpath.
    Subpath& close();

	///Returns the current subpath.
    const Subpath& currentSubpath() const { return subpaths_.back(); }

	///Returns the current subpath.
    Subpath& currentSubpath() { return subpaths_.back(); }

	///Returns the current point the path stays at.
	const Vec2f& currentPosition() const;

	///Returns a vector of all subpaths.
	const std::vector<Subpath>& subpaths() const { return subpaths_; }
};

//The Rectangle class represents a svg Rectangle shape.
class Rectangle : public ShapeBase
{
protected:
	Vec2f position_ {};
    Vec2f size_ {};
    Vec4f borderRadius_ {0.f, 0.f, 0.f, 0.f};

public:
    Rectangle(const Vec2f& position = Vec2f(), const Vec2f& size = Vec2f())
		: position_(position), size_(size) {}
	Rectangle(const Rect2f& rct)
		: position_(rct.position), size_(rct.size) {}

    void size(const Vec2f& size){ size_ = size; }
	const Vec2f& size() const { return size_; }

	void position(const Vec2f& position){ position_ = position; }
	const Vec2f& position() const { return position_; }

    void borderRadius(const Vec4f& br){ borderRadius_ = br; }
    void borderRadius(float value){ borderRadius_.fill(value); }
    const Vec4f& borderRadius() const { return borderRadius_; }

    Path asPath() const;

    template<class prec> operator Rect2<prec>() const { return Rect2<prec>(position(), size_); }
    virtual Rect2f extents() const { return Rect2f(position(), size_); }
};

//Text
class Text : public ShapeBase
{
public:
	enum class HorzBounds
	{
		left,
		center,
		right
	};

	enum class VertBounds
	{
		top,
		middle,
		bottom
	};

protected:
	Vec2f position_ {};
    float size_ {14};
    std::string string_ {};
    HorzBounds horzBounds_ {HorzBounds::left};
	VertBounds vertBounds_ {VertBounds::middle};
    Font* font_ {nullptr};

public:
    Text(const std::string& s = "", float size = 14);
    Text(const Vec2f& position, const std::string& s = "", float size = 14);

    void string(const std::string& s){ string_ = s; }
    const std::string& string() const { return string_; }

	void size(float s){ size_ = s; }
    float size() const { return size_; }

	void position(const Vec2f& position){ position_ = position; }
	const Vec2f& position() const { return position_; }

    HorzBounds horzBounds() const { return horzBounds_; }
    void horzBounds(HorzBounds b) { horzBounds_ = b; }

    VertBounds vertBounds() const { return vertBounds_; }
    void vertBounds(VertBounds b) { vertBounds_ = b; }

    Font* font() const { return font_; }
    void font(Font& f) { font_ = &f; }

    //inherited from transformable, TODO, requires font check
    virtual Rect2f extents() const { Rect2f ret; ret.position = position(); return ret; }
};

///The Circle class is able to hold a circle shape. It inherits the transformable2 class.
///There is no explicit ellipse class, one can use a Circle with matching scaling instead.
class Circle : public ShapeBase
{
protected:
	Vec2f center_ {0};
    float radius_ {0};

public:
    Circle(float radius = 0) : radius_(radius) {}
    Circle(const Vec2f& center, float radius = 0)
	 	: center_(center), radius_(radius) {}

    void radius(float r){ radius_ = r; }
    float radius() const { return radius_; }

	void center(const Vec2f& center){ center_ = center; }
	const Vec2f& center() const { return center_; }

    Path asPath() const;

    virtual Rect2f extents() const
		{ return Rect2f(center() - radius_, 2 * Vec2f(radius_, radius_)); }
};

//The PathBase is able to hold an Text, Rectangle, Circle or Path object. It can represent any
///svg shape.
class PathBase
{
public:
    enum class Type
    {
        text,
        rectangle,
        path,
        circle
    };

protected:
    Type type_;

    union
	{
		Text text_;
		Rectangle rectangle_;
		Circle circle_;
		Path path_;
	};

	///Calls the destructor on the current active type, so a new union member can be activated
	///(i.e. constructed).
	void resetUnion();

public:
	///Constructs the PathBase with the given type. The type parameter is defaulted to circle
	///since it is the most lightweight and easy type.
	PathBase(Type type = Type::circle) : type_(type), circle_() {};

    PathBase(const Rectangle& obj) : type_(Type::rectangle), rectangle_(obj) {}
    PathBase(const Circle& obj) : type_(Type::circle), circle_(obj) {}
    PathBase(const Text& obj) : type_(Type::text), text_(obj) {}
    PathBase(const Path& obj) : type_(Type::path), path_(obj) {}

	PathBase(const PathBase& other);
    PathBase& operator=(const PathBase& other);

	PathBase(PathBase&& other) noexcept;
    PathBase& operator=(PathBase&& other) noexcept;

    ~PathBase();

	///
	void text(const Text& obj);
	void rectangle(const Rectangle& obj);
	void circle(const Circle& obj);
	void path(const Path& obj);
	void path(Path&& obj);

	Type type() const { return type_; }

	const Text& text() const { return text_; }
	const Rectangle& rectangle() const { return rectangle_; }
	const Circle& circle() const { return circle_; }
	const Path& path() const { return path_; }

	const ShapeBase& shapeBase() const;
	ShapeBase& shapeBase();

    //transformable
    void rotate(const Vec<1, float>& rotation){ shapeBase().rotate(rotation); }
    void translate(const Vec2f& translation){ shapeBase().translate(translation); }
    void scale(const Vec2f& pscale){ shapeBase().scale(pscale); }

	void copyTransform(const Transform2& other)
		{ shapeBase().transformMatrix() = other.transformMatrix(); };
    const Mat3f transformMatrix() const { return shapeBase().transformMatrix(); }
};


///The Shape class is completley able to hold a svg shape tag like Rectangle, ellipse or path.
///It holds a path type which define the actual shape of the object as well as a brush and pen
///to fill or stroke the shape.
class Shape
{
protected:
    PathBase pathBase_;
    Brush brush_ = Brush::none;
    Pen pen_ = Pen::none;

public:
	Shape() = default;
	Shape(const PathBase& path, const Brush& brush = Brush::none, const Pen& pen = Pen::none)
		: pathBase_(path), brush_(brush), pen_(pen) {}

	const Pen& pen() const { return pen_; }
	const Brush& brush() const { return brush_; }
	const PathBase& pathBase() const { return pathBase_; }

	void pen(const Pen& p){ pen_ = p; }
	void brush(const Brush& b){ brush_ = b; }
	void pathBase(const PathBase& p){ pathBase_ = p; }
};

}
