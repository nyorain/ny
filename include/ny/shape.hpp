#pragma once

#include <ny/include.hpp>
#include <ny/color.hpp>
#include <nyutil/transformable.hpp>
#include <ny/font.hpp>

#include <nyutil/vec.hpp>
#include <nyutil/mat.hpp>
#include <nyutil/rect.hpp>
#include <nyutil/cache.hpp>

#include <vector>

namespace ny
{

//todo: match stl interface conventions

//enums
enum class drawStyle
{
    none,
    linear,
    arc,
    bezier
};

enum class textBound
{
    left,
    right,
    center
};

enum class pathType
{
    text,
    rectangle,
    circle,
    custom
};

//arcType
enum class arcType
{
    left,
    right,
    leftInverted,
    rightInverted
};

//tangentData
struct bezierData
{
	vec2f start;
	vec2f end;
};

//arcData
struct arcData
{
	float radius;
	arcType type;
};

//point
class point
{
protected:
	drawStyle style_;

	union
	{
		bezierData bezier_;
		arcData arc_;
	};

public:
	vec2f position;

	point() = default;
	point(const point& other) = default;
	point(const vec2f& pos) : position(pos) {}
	point(float x, float y) : position(x,y) {}

	drawStyle getDrawStyle() const { return style_; }
	bezierData getBezierData() const { if(style_ == drawStyle::bezier) return bezier_; return bezierData(); }
	arcData getArcData() const { if(style_ == drawStyle::arc) return arc_; return arcData(); }

	void setLinearDraw() { style_ = drawStyle::linear; }
	void setBezierDraw(bezierData d) { style_ = drawStyle::bezier; bezier_ = d; }
	void setBezierDraw(vec2f a, vec2f b) { style_ = drawStyle::bezier; bezier_.start = a; bezier_.end = b; }
	void setArcDraw(arcData d) { style_ = drawStyle::arc; arc_ = d; }
	void setArcDraw(float radius, arcType type) { style_ = drawStyle::arc; arc_.radius = radius; arc_.type = type; }
};

//posArray////////////////////////////////////////////////////////////
class vertexArray : public transformable2
{
protected:
    std::vector<vec2f> points_;

public:
    vec2f& operator[](size_t i){ return points_[i]; }
    const vec2f& operator[](size_t i) const { return points_[i]; }

    //inherited from transformable
    rect2f getExtents() const
    {
        rect2f ret;
        for(size_t i (0); i < points_.size(); i++)
        {
            ret.position.x = std::min(ret.left(), points_[i].x);
            ret.position.y = std::min(ret.top(), points_[i].y);
            ret.size.x = std::max(ret.size.x, points_[i].x - ret.position.x);
            ret.size.y = std::max(ret.size.y, points_[i].y - ret.position.y);
        }
        return ret;
    }
};

//path///////////////////////////////
//pathType of the first point will be ignored, its a startpoint
//the pathType of every point shows how to draw the line BEFORE the point
class customPath : public transformable2
{
protected:
    std::vector<point> points_ {};
    mutable vertexArray baked_ {};
    mutable unsigned int precision_ = 10;
    mutable bool needBake_ = 0;

public:
    customPath(vec2f start = vec2f());
    customPath(const customPath& other) = default;
    customPath(customPath&& other) = default;
    ~customPath() = default;

    customPath& operator=(const customPath& other) = default;
    customPath& operator=(customPath&& other) = default;

    point& operator[](size_t i){ return points_[i]; }
    const point& operator[](size_t i) const { return points_[i]; }

    size_t size() const { return points_.size(); }

    void addLine(vec2f p);
	void addLine(float x, float y);

	void addBezier(vec2f p, const bezierData& d);
	void addBezier(vec2f p, vec2f a, vec2f b);

	void addArc(vec2f p, const arcData& d);
	void addArc(vec2f p, float radius, arcType type);

	size_t addPoint(point p){ points_.push_back(p); return points_.size() - 1; }
	void removePoint(size_t pos){ points_.erase(points_.begin() + pos - 1); }

    //inherited from transformable
    rect2f getExtents() const { return getBaked().getExtents(); }

    void bake(int precision = -1) const; //default: use previous precision
    const std::vector<point>& getPoints() const { return points_; }
    const vertexArray& getBaked() const { if(needBake_)bake(); return baked_; }
    unsigned int getBakedPrecision() const { return precision_; }
};

//rectangle
class rectangle : public transformable2
{
protected:
    vec2f size_;
    vec4f borderRadius_;

public:
    rectangle(vec2f position = vec2f(), vec2f size = vec2f()) : size_(size) { setPosition(position); }
    rectangle(float px, float py, float sx = 0, float sy = 0) : size_(sx, sy) { setPosition({px,py}); }

    rectangle(const rectangle& other) = default;

    void setSize(vec2f size){ size_ = size; }
    void setSize(float width, float height){ size_ = {width, height}; }

    void setBorderRadius(vec4f br){ borderRadius_ = br; }
    void setBorderRadius(float value){ borderRadius_.allTo(value); }
    void setBorderRadius(float leftTop, float rightTop, float rightBottom, float leftBottom){ borderRadius_ = {leftTop, rightTop, rightBottom, leftBottom}; }

    vec2f getSize() const { return size_; }
    vec4f getBorderRadius() const { return borderRadius_; }

    customPath getAsCustomPath() const;

    template<class prec> operator rect2<prec>() const { return rect2<prec>(getPosition(), size_); }

    //inherited from transformable
    virtual rect2f getExtents() const override { return rect2f(getPosition(), size_); }
};

class text : public transformable2
{
protected:
    vec2f position_ {};
    float size_ {14};
    std::string string_ {};
    textBound bound_ {textBound::left};
    font* font_ {nullptr};

public:
    text(const std::string& s = "", float size = 14) : size_(size), string_(s), bound_(textBound::left), font_(&font::getDefault()) {}
    text(vec2f position, const std::string& s = "", float size = 14) : position_(position), size_(size), string_(s), font_(&font::getDefault()) {};
    text(float x, float y, float size) : position_(x,y), size_(size), font_(&font::getDefault()) {}

    void setPosition(vec2f position){ position_ = position; }
    void setPosition(float x, float y){ position_ = {x,y}; }

    void setString(const std::string& s){ string_ = s; }
    std::string getString() const { return string_; }

    vec2f getPosition() const { return position_; }
    float getSize() const { return size_; }

    textBound getBound() const { return bound_; }
    void setBound(textBound b) { bound_ = b; }

    font* getFont() const { return font_; }
    void setFont(font& f) { font_ = &f; }

    //inherited from transformable
    rect2f getExtents() const { rect2f ret; return ret; }
};

class circle : public transformable2
{
protected:
    vec2f position_ {};
    float radius_ {0};
    unsigned int points_ {0};

public:
    circle() = default;
    circle(float radius) : radius_(radius) {}
    circle(vec2f position, float radius = 0, unsigned int points = 30) : position_(position), radius_(radius), points_(points) {}
    circle(float x, float y, float radius = 0, unsigned int points = 30) : position_(x,y), radius_(radius), points_(points) {}

    circle(const circle& other) = default;

    void setPosition(vec2f position){ position_ = position; }
    void setPosition(float x, float y){ position_ = {x,y}; }

    void setRadius(float r){ radius_ = r; }
    void setPoints(unsigned int points){ points_ = points; }

    vec2f getPosition() const { return position_; }
    float getRadius() const { return radius_; }
    unsigned int getPoints() const { return points_; }

    vec2f getCenter() const { return getPosition() - getOrigin() + vec2f(radius_, radius_); } //todo?

    customPath getAsCustomPath() const;

    //inherited from transformable
    rect2f getExtents() const { return rect2f(position_, radius_ * 2); }
};

//path///////////////////////////////////////////////////////////////
class path
{
protected:
    pathType type_;

    union
	{
		text text_;
		rectangle rectangle_;
		circle circle_;
		customPath custom_;
	};

public:
	path() : type_(pathType::custom), custom_() {}

    path(const rectangle& obj) : type_(pathType::rectangle), rectangle_(obj) {}
    path(const circle& obj) : type_(pathType::circle), circle_(obj) {}
    path(const text& obj) : type_(pathType::text), text_(obj) {}
    path(const customPath& obj) : type_(pathType::custom), custom_(obj) {}

	path(const path& lhs) noexcept;
    path& operator=(const path& lhs) noexcept;

	path(path&& lhs) noexcept;
    path& operator=(path&& lhs) noexcept;

    ~path();

	void setText(const text& obj){ type_ = pathType::text; text_ = obj; }
	void setRectangle(const rectangle& obj){ type_ = pathType::rectangle; rectangle_ = obj; }
	void setCircle(const circle& obj){ type_ = pathType::circle; circle_ = obj; }
	void setCustom(const customPath& obj){ type_ = pathType::custom; custom_ = obj; }

	pathType getPathType() const { return type_; }

	const text& getText() const { return text_; }
	const rectangle& getRectangle() const { return rectangle_; }
	const circle& getCircle() const { return circle_; }
	const customPath& getCustom() const { return custom_; }

	const transformable2& getTransformable() const;
	transformable2& getTransformable();

    ////transformable//////////////
    void rotate(float rotation){ getTransformable().rotate(rotation); }
    void move(const vec2f& translation){ getTransformable().move(translation); }
    void scale(const vec2f& pscale){ getTransformable().scale(pscale); }
    void moveOrigin(const vec2f& m) { getTransformable().moveOrigin(m); };

    void setRotation(float rotation){ getTransformable().setRotation(rotation); }
    void setPosition(const vec2f& translation){ getTransformable().setPosition(translation); }
    void setScale(const vec2f& pscale){ getTransformable().setScale(pscale); }
    void setOrigin(const vec2f& origin) { getTransformable().setOrigin(origin); }

    float getRotation() const { return getTransformable().getRotation(); }
    const vec2f& getPosition() const { return getTransformable().getPosition(); }
    const vec2f& getScale() const { return getTransformable().getScale(); }
    const vec2f& getOrigin() const { return getTransformable().getOrigin(); }

	void copyTransform(const transformable2& other){ getTransformable().copyTransform(other); };

    mat3f getTransformMatrix() const { return getTransformable().getTransformMatrix(); }

	rect2f getExtents() const { return getTransformable().getExtents(); }
	rect2f getTransformedExtents() const { return getTransformable().getTransformedExtents(); }

};

//shape///////////////////////////////////////
class mask : public std::vector<path>
{
public:
	mask() = default;

/*
	const std::vector<path>& paths() const { return paths_; }

	void addPath(const path& p){ paths_.emplace_back(p); }
	void addPath(path&& p){ paths_.push_back(std::move(p)); }
*/
    rect2f getExtents() const { return rect2f(); }
};

//shape//////////////////////////////////////
class shape
{
protected:
    mask mask_;
    brush brush_;
    pen pen_;

    bool hasBrush_ {0};
    bool hasPen_ {0};

public:
    shape(const mask& m = mask()) : mask_(m) {}

    const mask& getMask() const { return mask_; }
    const brush* getBrush() const { return (hasBrush_) ? &brush_ : nullptr; }
    const pen* getPen() const { return (hasPen_) ? &pen_ : nullptr; }

    mask& getMask() { return mask_; }
    brush* getBrush() { return (hasBrush_) ? &brush_ : nullptr; }
    pen* getPen() { return (hasPen_) ? &pen_ : nullptr; }

    void setPen(const pen* p){ if(p){ hasPen_ = 1; pen_ = *p; } }
    void setBrush(const brush* b){ if(b){ hasBrush_ = 1; brush_ = *b; } }
};

//customShape//////////////////////////////////
std::vector<vec2f> bakePoints(const std::vector<point>& vec);



}
