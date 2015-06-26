#pragma once

#include "include.hpp"
#include "color.hpp"
#include "transformable.hpp"
#include "font.hpp"
#include <vector>
#include <utils/vec.hpp>
#include <utils/mat.hpp>
#include <utils/rect.hpp>
;

namespace ny
{

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
    middle
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
	bezierData getTangentData() const { if(style_ == drawStyle::bezier) return bezier_; return bezierData(); }
	arcData getArcData() const { if(style_ == drawStyle::arc) return arc_; return arcData(); }

	void setLinearDraw() { style_ = drawStyle::linear; }
	void setBezierDraw(bezierData d) { style_ = drawStyle::bezier; bezier_ = d; }
	void setBezierDraw(vec2f a, vec2f b) { style_ = drawStyle::bezier; bezier_.start = a; bezier_.end = b; }
	void setArcDraw(arcData d) { style_ = drawStyle::arc; arc_ = d; }
	void setArcDraw(float radius, arcType type) { style_ = drawStyle::arc; arc_.radius = radius; arc_.type = type; }
};

//posArray////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class pointArray : public transformable2
{
protected:
    std::vector<vec2f> points_;
public:
    pointArray() = default;
    pointArray(const pointArray& other) = default;
    ~pointArray() = default;

    vec2f& operator[](size_t i){ return points_[i]; }
    const vec2f& operator[](size_t i) const { return points_[i]; }

    //inherited from transformable
    rect2f getExtents() const { rect2f ret; for(size_t i (0); i < points_.size(); i++){
        ret.position.x = std::min(ret.left(), points_[i].x);
        ret.position.y = std::min(ret.top(), points_[i].y);
        ret.size.x = std::max(ret.size.x, points_[i].x - ret.position.x);
        ret.size.y = std::max(ret.size.y, points_[i].y - ret.position.y);
        } return ret; }
};

//path////////////////////////////////////////////////////////////7
//rectangle
class rectangle : public transformable2
{
protected:
    vec2f position_;
    vec2f size_;
    vec4f borderRadius_;

public:
    rectangle(vec2f position = vec2f(), vec2f size = vec2f()) : position_(position), size_(size) {}
    rectangle(float px, float py, float sx = 0, float sy = 0) : position_(px, py), size_(sx, sy) {}

    rectangle(const rectangle& other) = default;

    void setPosition(vec2f position){ position_ = position; }
    void setPosition(float x, float y){ position_ = {x,y}; }

    void setSize(vec2f size){ size_ = size; }
    void setSize(float width, float height){ size_ = {width, height}; }

    void setBorderRadius(vec4f br){ borderRadius_ = br; }
    void setBorderRadius(float value){ borderRadius_.allTo(value); }
    void setBorderRadius(float leftTop, float rightTop, float rightBottom, float leftBottom){ borderRadius_ = {leftTop, rightTop, rightBottom, leftBottom}; }

    vec2f getPosition() const { return position_; }
    vec2f getSize() const { return size_; }
    vec4f getBorderRadius() const { return borderRadius_; }

    template<class prec> operator rect2<prec>() const { return rect2<prec>(position_, size_); }

    //inherited from transformable
    rect2f getExtents() const { return *this; }
};

class text : public transformable2
{
protected:
    vec2f position_;
    float size_;
    std::string string_;
    textBound bound_;
    font* font_;

public:
    text() : size_(14), bound_(textBound::left), font_(font::getDefault()) {}
    text(vec2f position, const std::string& s = "", float size = 14) : position_(position), size_(size), string_(s), font_(font::getDefault()) {};
    text(float x, float y, float size) : position_(x,y), size_(size), font_(font::getDefault()) {}

    ~text() = default;
    text(const text& other) = default;


    void setPosition(vec2f position){ position_ = position; }
    void setPosition(float x, float y){ position_ = {x,y}; }

    void setString(const std::string& s){ string_ = s; }
    std::string getString() const { return string_; }

    vec2f getPosition() const { return position_; }
    float getSize() const { return size_; }

    //inherited from transformable
    rect2f getExtents() const { rect2f ret; return ret; }
};

class circle : public transformable2
{
protected:
    vec2f position_;
    float radius_;
    unsigned int points_;

public:
    circle(float radius = 0, unsigned int points = 30) : radius_(radius), points_(points) {};
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

    //inherited from transformable
    rect2f getExtents() const { return rect2f(position_, radius_ * 2); }
};

class customPath : public transformable2
{
protected:
    std::vector<point> points_;
    mutable pointArray baked_;
    mutable unsigned int precision_ = 10;
    mutable bool needBake_ = 0;

public:
    customPath(vec2f start = vec2f());
    customPath(const customPath& other) = default;
    customPath(customPath&& other) = default;
    ~customPath() = default;

    customPath& operator=(const customPath& other) = default;

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
    const pointArray& getBaked() const { if(needBake_)bake(); return baked_; }
    unsigned int getBakedPrecision() const { return precision_; }
};

//path/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
	path(const path& other) : type_(other.type_) { if(type_ == pathType::text)text_ = other.text_; else if(type_ == pathType::rectangle)rectangle_ = other.rectangle_; else if(type_ == pathType::circle)circle_ = other.circle_; else if(type_ == pathType::custom)custom_ = other.custom_; }
    ~path(){ }

	void setText(const text& obj){ type_ = pathType::text; text_ = obj; }
	void setRectangle(const rectangle& obj){ type_ = pathType::rectangle; rectangle_ = obj; }
	void setCircle(const circle& obj){ type_ = pathType::circle; circle_ = obj; }
	void setCustom(const customPath& obj){ type_ = pathType::custom; custom_ = obj; }

	pathType getPathType() const { return type_; }

	const text& getText() const { if(type_ == pathType::text) return text_; }
	const rectangle& getRectangle() const { if(type_ == pathType::rectangle) return rectangle_; }
	const circle& getCircle() const { if(type_ == pathType::circle) return circle_; }
	const customPath& getCustom() const { if(type_ == pathType::custom) return custom_; }

	const transformable2& getObject() const { if(type_ == pathType::text) return text_; else if(type_ == pathType::rectangle) return rectangle_; else if(type_ == pathType::circle) return circle_; else return custom_;}
	transformable2& getObject() { if(type_ == pathType::text) return text_; else if(type_ == pathType::rectangle) return rectangle_; else if(type_ == pathType::circle) return circle_; else return custom_;}

    //transform////////////////////////////
    void rotate(float rotation){ getObject().rotate(rotation); }
    void translate(const vec2f& translation){ getObject().translate(translation); }
    void scale(const vec2f& pscale){ getObject().scale(pscale); }

    void setRotation(float rotation){ getObject().setRotation(rotation); }
    void setTranslation(const vec2f& translation){ getObject().setTranslation(translation); }
    void setScale(const vec2f& pscale){ getObject().setScale(pscale); }

    float getRotation() const { return getObject().getRotation(); }
    const vec2f& getTranslation() const { return getObject().getTranslation(); }
    const vec2f& getScale() const { return getObject().getScale(); }

	const vec2f& getOrigin() const { return getObject().getOrigin(); }
	void setOrigin(const vec2f& origin) { getObject().setOrigin(origin); }
	void moveOrigin(const vec2f& m) { getObject().moveOrigin(m); };

	void copyTransform(const transformable2& other){ getObject().copyTransform(other); };

    mat3f getTransformMatrix() const { return getObject().getTransformMatrix(); }

	rect2f getExtents() const { return getObject().getExtents(); }
	rect2f getTransformedExtents() const { return getObject().getTransformedExtents(); }

};

//shape//////////////////////////////////////////////////////////////////////////////
class mask
{
protected:
    std::vector<path> paths_;

public:
	mask() = default;

	const std::vector<path>& paths() const { return paths_; }
};

//shape///////////////////////////////////////////////////////////////////////////
class shape : public transformable2
{
protected:
    mask mask_;
    brush brush_;
    pen pen_;

public:
    shape() = default;

    const mask& getMask() const { return mask_; }
    const brush& getBrush() const { return brush_; }
    const pen& getPen() const { return pen_; }
};

//customShape///////////////////////////////////////////////////////////////////
std::vector<vec2f> bakePoints(const std::vector<point>& vec);



}
