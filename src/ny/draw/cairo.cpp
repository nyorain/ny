#include <ny/draw/cairo.hpp>
#include <ny/base/image.hpp>
#include <ny/draw/font.hpp>

#include <ny/base/log.hpp>
#include <nytl/cache.hpp>
#include <nytl/clone.hpp>

#include <cairo/cairo.h>

#include <cmath>

//macro for current context validation
#if defined(__GNUC__) || defined(__clang__)
 #define FUNC_NAME __PRETTY_FUNCTION__
#else
 #define FUNC_NAME __func__
#endif //FUNCNAME

//assuring cairo
#define VALIDATE_CTX(x) if(!cairoCR_)\
	{ sendWarning(FUNC_NAME, ": no cairoCR handle."); return x; }

namespace ny
{

//Cache Name: "ny::CairoFontHandle"
class CairoFontHandle : public Cache
{
protected:
    cairo_font_face_t* handle_;

public:
    CairoFontHandle(const Font& font) : CairoFontHandle(font.name(), font.fromFile()) {}
    CairoFontHandle(const std::string& name, bool fromFile = 0);
    ~CairoFontHandle();

    cairo_font_face_t* cairoFontFace() const { return handle_; }
};


//Font
CairoFontHandle::CairoFontHandle(const std::string& name, bool)
{
    handle_ = cairo_toy_font_face_create(name.c_str(), CAIRO_FONT_SLANT_NORMAL, 
			CAIRO_FONT_WEIGHT_NORMAL);

    if(!handle_)
        sendWarning("CairoFontHandle: failed to create cairo font");
}

CairoFontHandle::~CairoFontHandle()
{
    if(handle_) cairo_font_face_destroy(handle_);
}

//util
enum class direction
{
    Left,
    Right
};

Vec2d circleCenter(Vec2d p1, Vec2d p2,double radius, direction m)
{
    double distance = sqrt(pow(p2.x - p1.x,2) + pow(p2.y - p1.y, 2));

    Vec2d p3;
    p3.x = (p1.x + p2.x) / 2;
    p3.y = (p1.y + p2.y) / 2;

    Vec2d ret;
    if(m == direction::Left)
    {
        ret.x = p3.x - sqrt(pow(radius,2) - pow(distance / 2, 2)) * (p1.y - p2.y)/ distance;
        ret.y = p3.y - sqrt(pow(radius,2) - pow(distance / 2, 2)) * (p2.x - p1.x)/ distance;
    }

    else
    {
        ret.x = p3.x + sqrt(pow(radius,2) - pow(distance / 2, 2)) * (p1.y - p2.y)/ distance;
        ret.y = p3.y + sqrt(pow(radius,2) - pow(distance / 2, 2)) * (p2.x - p1.x)/ distance;
    }

    return ret;
}


//Cairo DC Implementation
CairoDrawContext::CairoDrawContext()
{
}

CairoDrawContext::CairoDrawContext(cairo_surface_t& cairoSurface)
{
	cairoSurface_ = cairo_surface_reference(&cairoSurface);
	cairoCR_ = cairo_create(cairoSurface_);
}

CairoDrawContext::CairoDrawContext(Image& img)
{
	//todo: corRect format and stuff
    cairoSurface_ = cairo_image_surface_create_for_data(img.data(), CAIRO_FORMAT_ARGB32, 
		img.size().x, img.size().y, img.size().x * 4);
	cairoCR_ = cairo_create(cairoSurface_);
}

CairoDrawContext::~CairoDrawContext()
{
	if(cairoCR_)
	{
		cairo_destroy(cairoCR_);
	}

	if(cairoSurface_)
	{
		//will only remove the reference
		cairo_surface_destroy(cairoSurface_);
	}
}

void CairoDrawContext::create(cairo_surface_t& surface)
{
	if(cairoCR_) cairo_destroy(cairoCR_);

	cairoSurface_ = cairo_surface_reference(&surface);
	cairoCR_ = cairo_create(cairoSurface_);
}

void CairoDrawContext::clear(const Brush& brush)
{
	VALIDATE_CTX();
    auto col = brush.color().rgbaNorm();

    cairo_save(cairoCR_);
    cairo_set_source_rgba(cairoCR_, col.x, col.y, col.z, col.w);
    cairo_paint(cairoCR_);
    cairo_restore(cairoCR_);
}

void CairoDrawContext::paint(const Brush& brush, const Brush& alpha)
{
	VALIDATE_CTX();
	//TODO
}

void CairoDrawContext::apply()
{
	VALIDATE_CTX();

	cairo_surface_flush(cairoSurface_);
    cairo_show_page(cairoCR_);
}

Rect2f CairoDrawContext::rectangleClip() const
{
    Rect2f ret;

	VALIDATE_CTX(ret);

    cairo_rectangle_list_t* recList = cairo_copy_clip_rectangle_list(cairoCR_);
    if(recList->num_rectangles == 0)
        return Rect2f();

    cairo_rectangle_t& r = recList->rectangles[0];
    ret.position = Vec2f(r.x, r.y);
    ret.size =  Vec2f(r.width, r.height);

    return ret;
}

void CairoDrawContext::clipRectangle(const Rect2f& obj)
{
	VALIDATE_CTX();

    cairo_rectangle(cairoCR_, obj.left(), obj.top(), obj.width(), obj.height());
    cairo_clip(cairoCR_);
}

void CairoDrawContext::resetRectangleClip()
{
	VALIDATE_CTX();
    cairo_reset_clip(cairoCR_);
}

void CairoDrawContext::applyTransform(const Transform2& xtransform)
{
	VALIDATE_CTX();

    cairo_matrix_t tm {};
    auto& om = xtransform.transformMatrix();

    cairo_matrix_init(&tm, om[0][0], om[1][0], om[0][1], om[1][1], om[0][2], om[1][2]);
    cairo_set_matrix(cairoCR_, &tm);
}

void CairoDrawContext::resetTransform()
{
	VALIDATE_CTX();
    cairo_identity_matrix(cairoCR_);
}

void CairoDrawContext::mask(const Text& obj)
{
	VALIDATE_CTX();

    if(!obj.font() || obj.string().empty())
        return;

    applyTransform(obj);

	auto font = static_cast<const CairoFontHandle*>(obj.font()->cache("ny::CairoFontHandle"));
	if(!font)
	{
		auto created = std::make_shared<CairoFontHandle>(*obj.font());
		font = created.get();
		obj.font()->cache("ny::CairoFontHandle", std::move(created));
	}

    cairo_set_font_face(cairoCR_, font->cairoFontFace());
    cairo_set_font_size(cairoCR_, obj.size());

	Vec2i position = obj.position();
	cairo_text_extents_t extents;
	cairo_text_extents(cairoCR_, obj.string().c_str(), &extents);

	if(obj.horzBounds() == Text::HorzBounds::center)
	{
		position.x -= extents.width / 2 + extents.x_bearing;
	}
	if(obj.vertBounds() == Text::VertBounds::middle)
	{
		position.y -= extents.height / 2 + extents.y_bearing;
	}

    cairo_move_to(cairoCR_, position.x, position.y);
    cairo_text_path(cairoCR_, obj.string().c_str());

    resetTransform();
}

void CairoDrawContext::mask(const Rectangle& obj)
{
	VALIDATE_CTX();

    if(all(obj.size() == Vec2f()))
        return;

    applyTransform(obj);
    if(all(obj.borderRadius() == 0)) //no border radius
    {
        cairo_rectangle(cairoCR_, 0, 0, obj.size().x, obj.size().y);
    }
    else
    {
        cairo_move_to(cairoCR_, 0, 0);
        cairo_arc(cairoCR_, obj.size().x - obj.borderRadius().x, obj.borderRadius().x, 
				obj.borderRadius().x, -90 * cDeg, 0 * cDeg);
        cairo_arc(cairoCR_, obj.size().x - obj.borderRadius().y, 
				obj.size().y - obj.borderRadius().y, obj.borderRadius().y, 0 * cDeg, 90 * cDeg);
        cairo_arc(cairoCR_, obj.borderRadius().z, obj.size().y - obj.borderRadius().z, 
				obj.borderRadius().z, 90 * cDeg, 180 * cDeg);
        cairo_arc(cairoCR_, obj.borderRadius().w, obj.borderRadius().w, obj.borderRadius().w, 
				180 * cDeg, 270 * cDeg);
    }

    resetTransform();
}

void CairoDrawContext::mask(const Circle& obj)
{
	VALIDATE_CTX();

    if(obj.radius() == 0)
        return;

    applyTransform(obj);
    cairo_arc(cairoCR_, obj.radius(), obj.radius(), obj.radius(), 0, 360 * cDeg);

    resetTransform();
}

void CairoDrawContext::mask(const Path& obj)
{
	VALIDATE_CTX();

    if(obj.subpaths().size() <= 1)
        return;

    applyTransform(obj);

	//TODO!
	/*
    std::vector<point> points = obj.getPoints();

    for(unsigned int i(0); i < points.size(); i++)
    {
        //beginning
        if(i == 0)
        {
            cairo_move_to(cairoCR_, points[0].position.x, points[0].position.y);
            continue;
        }

        //linear
        if(points[i].getDrawStyle() == drawStyle::linear)
        {
            cairo_line_to(cairoCR_, points[i].position.x, points[i].position.y);
            continue;
        }

        //tangent
        else if(points[i].getDrawStyle() == drawStyle::bezier)
        {
            bezierData data = points[i].getBezierData();
            cairo_curve_to(cairoCR_, data.start.x, data.start.y, data.end.x, data.start.y, points[i].position.x, points[i].position.y);
            continue;
        }

        //arc
        else if(points[i].getDrawStyle() == drawStyle::arc)
        {
            //TODO

            Vec2f p1 = points[i - 1].position;

            Vec2f center;
            double angle1 = 0, angle2 = 0;

            if(points[i].getArcData().type ==  arcType::left || points[i].getArcData().type == arcType::leftInverted)
            {
                center = circleCenter((Vec2f) p1, points[i].position, points[i].getArcData().radius, diRection::Left);

                angle1 += M_PI / 2;
                angle2 += M_PI / 2;
            }
            else
            {
                center = circleCenter((Vec2f) p1, points[i].position, points[i].getArcData().radius, diRection::Right);
            }

            if(p1.y != center.y) angle1 += asin((p1.y - center.y) / points[i].getArcData().radius);
            if(points[i].position.y != center.y) angle2 += asin((points[i].position.y - center.y) / points[i].getArcData().radius);

            if(points[i].getArcData().type == arcType::left)
            {
                std::swap(angle1, angle2);
                cairo_arc_negative(cairoCR_, center.x, center.y, points[i].getArcData().radius, angle1, angle2);
            }

            else if(points[i].getArcData().type == arcType::leftInverted)
            {
                std::swap(angle1, angle2);
                cairo_arc(cairoCR_, center.x, center.y, points[i].getArcData().radius, angle1, angle2);
            }

            else if(points[i].getArcData().type == arcType::rightInverted)
            {
                cairo_arc_negative(cairoCR_, center.x, center.y, points[i].getArcData().radius, angle1, angle2);
            }

            else
            {
                cairo_arc(cairoCR_, center.x, center.y, points[i].getArcData().radius, angle1, angle2);
            }

        }
    }
	*/
}

void CairoDrawContext::resetMask()
{
	VALIDATE_CTX();

    //todo - better way?
    cairo_set_source_rgba(cairoCR_, 0, 0, 0, 0); //nothing
    cairo_fill(cairoCR_); //just clean current mask
}

void CairoDrawContext::strokePreserve(const Pen& pen)
{
	VALIDATE_CTX();

    auto col = pen.brush().color().rgbaNorm();

    cairo_set_source_rgba(cairoCR_, col.x, col.y, col.z, col.w);
    cairo_stroke_preserve(cairoCR_);
}

void CairoDrawContext::fillPreserve(const Brush& brush)
{
	VALIDATE_CTX();

    auto col = brush.color().rgbaNorm();

    cairo_set_source_rgba(cairoCR_, col.x, col.y, col.z, col.w);
    cairo_fill_preserve(cairoCR_);
}

void CairoDrawContext::stroke(const Pen& pen)
{
	VALIDATE_CTX();

    auto col = pen.brush().color().rgbaNorm();

    cairo_set_source_rgba(cairoCR_, col.x, col.y, col.z, col.w);
    cairo_stroke(cairoCR_);
}

void CairoDrawContext::fill(const Brush& brush)
{
	VALIDATE_CTX();

    auto col = brush.color().rgbaNorm();

    cairo_set_source_rgba(cairoCR_, col.x, col.y, col.z, col.w);
    cairo_fill(cairoCR_);
}

}
