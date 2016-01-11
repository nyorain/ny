#include <ny/draw/cairo.hpp>
#include <ny/draw/image.hpp>

#include <nytl/log.hpp>
#include <nytl/cache.hpp>

#include <cairo/cairo.h>

#include <cmath>

namespace ny
{

//Cache Name: "ny::CairoFontHandle"
class CairoFontHandle : public cache
{
protected:
    cairo_font_face_t* handle_;

public:
    CairoFontHandle(const std::string& name, bool fromFile = 0);
    ~CairoFontHandle();

    cairo_font_face_t* getFontFace() const { return handle_; }
};


//Font
CairoFontHandle::CairoFontHandle(const std::string& name, bool)
{
    handle_ = cairo_toy_font_face_create(name.c_str(), CAIRO_FONT_SLANT_NORMAL, 
			CAIRO_FONT_WEIGHT_NORMAL);

    if(!handle_)
        nytl::sendWarning("CairoFontHandle: failed to create cairo font");
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

vec2d circleCenter(vec2d p1, vec2d p2,double radius, direction m)
{
    double distance = sqrt(pow(p2.x - p1.x,2) + pow(p2.y - p1.y, 2));

    vec2d p3;
    p3.x = (p1.x + p2.x) / 2;
    p3.y = (p1.y + p2.y) / 2;

    vec2d ret;
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
	//todo: correct format and stuff
    cairoSurface_ = cairo_image_surface_create_for_data(img.data().data(), CAIRO_FORMAT_ARGB32, 
		img.size().x, img.size().y, img.size().x * 4);
}

CairoDrawContext::~CairoDrawContext()
{
	if(cairoSurface_)
	{
		//will only remove the reference
		cairo_surface_destroy(cairoSurface_);
	}
}

void CairoDrawContext::clear(const Brush& b)
{
    if(!cairoCR_)
    {
		nytl::sendWarning("drawing with uninitialized cairoDC");
        return;
    }

    float r, g, b, a = 0;
    col.normalized(r, g, b, a);

    cairo_save (cairoCR_);
    cairo_set_source_rgba (cairoCR_, r, g, b, a);
    cairo_set_operator (cairoCR_, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cairoCR_);
    cairo_restore (cairoCR_);
}

void CairoDrawContext::apply()
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    cairo_show_page(cairoCR_);

    cairo_destroy(cairoCR_);
    cairoCR_ = cairo_create(cairoSurface_);
}

rect2f CairoDrawContext::rectangleClip() const
{
    rect2f ret;

    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return ret;
    }

    cairo_rectangle_list_t* recList = cairo_copy_clip_rectangle_list(cairoCR_);
    if(recList->num_rectangles == 0)
        return rect2f();

    cairo_rectangle_t& r = recList->rectangles[0];
    ret.position = vec2f(r.x, r.y);
    ret.size =  vec2f(r.width, r.height);

    return ret;
}

void CairoDrawContext::clipRectangle(const rect2f& obj)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    cairo_rectangle(cairoCR_, obj.left(), obj.top(), obj.width(), obj.height());
    cairo_clip(cairoCR_);
}

void CairoDrawContext::resetRectangleClip()
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    cairo_reset_clip(cairoCR_);
}

void CairoDrawContext::applyTransform(const transform2& xtransform)
{
    cairo_matrix_t tm {};
    auto& om = obj.getTransformMatrix();

    cairo_matrix_init(&tm, om[0][0], om[1][0], om[0][1], om[1][1], om[0][2], om[1][2]);
    cairo_set_matrix(cairoCR_, &tm);
}

void CairoDrawContext::resetTransform()
{
    cairo_identity_matrix(cairoCR_);
}

void CairoDrawContext::mask(const Text& obj)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    if(!obj.getFont() || obj.getString().empty())
        return;


    applyTransform(obj);

    cairo_move_to(cairoCR_, obj.getPosition().x, obj.getPosition().y);

    if(obj.getFont()->getCairoHandle(1) && obj.getFont()->getCairoHandle()->getFontFace())
        cairo_set_font_face(cairoCR_, obj.getFont()->getCairoHandle()->getFontFace());
    else
        nyWarning("cairo: could not use selected font");

    cairo_set_font_size(cairoCR_, obj.getSize());
    cairo_text_path(cairoCR_, obj.getString().c_str());

    resetTransform();
}

void CairoDrawContext::mask(const Rectangle& obj)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    if(obj.getSize() == vec2f())
        return;

    applyTransform(obj);

    if(obj.getBorderRadius() == vec4f()) //no border radius
    {
        vec2f pos123{0, 0};
        cairo_rectangle(cairoCR_, pos123.x, pos123.y, obj.getSize().x, obj.getSize().y);
    }
    else
    {
        vec2f pos123{0, 0};
        cairo_move_to(cairoCR_, pos123.x, pos123.y);
        cairo_arc(cairoCR_, pos123.x + obj.getSize().x - obj.getBorderRadius().x, pos123.y + obj.getBorderRadius().x, obj.getBorderRadius().x, -90 * cDeg, 0 * cDeg);
        cairo_arc(cairoCR_, pos123.x + obj.getSize().x - obj.getBorderRadius().y, pos123.y + obj.getSize().y - obj.getBorderRadius().y, obj.getBorderRadius().y, 0 * cDeg, 90 * cDeg);
        cairo_arc(cairoCR_, pos123.x + obj.getBorderRadius().z, pos123.y + obj.getSize().y - obj.getBorderRadius().z, obj.getBorderRadius().z, 90 * cDeg, 180 * cDeg);
        cairo_arc(cairoCR_, pos123.x + obj.getBorderRadius().w, pos123.y + obj.getBorderRadius().w, obj.getBorderRadius().w, 180 * cDeg, 270 * cDeg);
    }

    resetTransform();
}

void CairoDrawContext::mask(const Circle& obj)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    if(obj.getRadius() == 0)
        return;

    applyTransform(obj);

    cairo_arc(cairoCR_, obj.getCenter().x, obj.getCenter().y, obj.getRadius(), 0, 360 * cDeg);

    resetTransform();
}

void CairoDrawContext::mask(const Path& obj)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    if(obj.getPoints().size() <= 1)
        return;

    applyTransform(obj);

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

            vec2f p1 = points[i - 1].position;

            vec2f center;
            double angle1 = 0, angle2 = 0;

            if(points[i].getArcData().type ==  arcType::left || points[i].getArcData().type == arcType::leftInverted)
            {
                center = circleCenter((vec2f) p1, points[i].position, points[i].getArcData().radius, direction::Left);

                angle1 += M_PI / 2;
                angle2 += M_PI / 2;
            }
            else
            {
                center = circleCenter((vec2f) p1, points[i].position, points[i].getArcData().radius, direction::Right);
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
}

void CairoDrawContext::resetMask()
{
    //todo
    cairo_set_source_rgba(cairoCR_, 0, 0, 0, 0); //nothing
    cairo_fill(cairoCR_); //just clean current mask
}

void CairoDrawContext::strokePreserve(const Brush& col)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    float r, g, b, a = 0;
    col.normalized(r, g, b, a);

    cairo_set_source_rgba(cairoCR_, r, g, b, a);
    cairo_stroke_preserve(cairoCR_);
}

void CairoDrawContext::fillPreserve(const Pen& col)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    float r, g, b, a = 0;
    col.normalized(r, g, b, a);

    cairo_set_source_rgba(cairoCR_, r, g, b, a);
    cairo_fill_preserve(cairoCR_);
}

void CairoDrawContext::stroke(const Brush& col)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    float r, g, b, a = 0;
    col.normalized(r, g, b, a);

    cairo_set_source_rgba(cairoCR_, r, g, b, a);
    cairo_stroke(cairoCR_);
}

void CairoDrawContext::fill(const Pen& col)
{
    if(!cairoCR_)
    {
        nyWarning("drawing with uninitialized cairoDC");
        return;
    }

    float r, g, b, a = 0;
    col.normalized(r, g, b, a);

    cairo_set_source_rgba(cairoCR_, r, g, b, a);
    cairo_fill(cairoCR_);
}

/*
//general cairo functions//////////////////////////////////////////////////////////////////////////////////////////////
void cairoRect(cairo_t* cr, const rectangle& obj)
{

    float r = (float) obj.fillColor.r / 255.0;
    float g = (float) obj.fillColor.g / 255.0;
    float b = (float) obj.fillColor.b / 255.0;
    float a = (float) obj.fillColor.a / 255.0;

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, obj.position.x, obj.position.y, obj.size.x, obj.size.y);
    cairo_fill(cr);

    r = (float) obj.borderColor.r / 255.0;
    g = (float) obj.borderColor.g / 255.0;
    b = (float) obj.borderColor.b / 255.0;
    a = (float) obj.borderColor.a / 255.0;

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_set_line_width(cr, obj.borderWidth);
    cairo_stroke (cr);

}

void cairoRoundedRect(cairo_t* cr,const  rectangle& obj)
{

        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    double degrees = Pi / 180.0;

    //cairo_new_sub_path (cr);
    cairo_arc (cr, obj.position.x + obj.size.x - obj.borderRadius.a(), obj.position.y + obj.borderRadius.a(), obj.borderRadius.a(), -90 * degrees, 0 * degrees);
    cairo_arc (cr, obj.position.x + obj.size.x - obj.borderRadius.b(), obj.position.y + obj.size.y - obj.borderRadius.b(), obj.borderRadius.b(), 0 * degrees, 90 * degrees);
    cairo_arc (cr, obj.position.x + obj.borderRadius.c(), obj.position.y + obj.size.y - obj.borderRadius.c(), obj.borderRadius.c(), 90 * degrees, 180 * degrees);
    cairo_arc (cr, obj.position.x + obj.borderRadius.d(), obj.position.y + obj.borderRadius.d(), obj.borderRadius.d(), 180 * degrees, 270 * degrees);
    cairo_close_path (cr);

    float r = (float) obj.fillColor.r / 255.0;
    float g = (float) obj.fillColor.g / 255.0;
    float b = (float) obj.fillColor.b / 255.0;
    float a = (float) obj.fillColor.a / 255.0;

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_fill(cr);

    r = (float) obj.borderColor.r / 255.0;
    g = (float) obj.borderColor.g / 255.0;
    b = (float) obj.borderColor.b / 255.0;
    a = (float) obj.borderColor.a / 255.0;

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_set_line_width(cr, obj.borderWidth);
    cairo_stroke (cr);

}

void cairoCircle(cairo_t* cr, const circle& obj)
{

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    float r = (float) obj.fillColor.r / 255;
    float g = (float) obj.fillColor.g / 255;
    float b = (float) obj.fillColor.b / 255;
    float a = (float) obj.fillColor.a / 255;

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_arc(cr, obj.position.x, obj.position.y, obj.radius, 0, 2 * 3.14159265359);

    cairo_fill(cr);

}

void cairoEllipse(cairo_t* cr,const  ellipse& obj)
{

}

void cairoText(cairo_t* cr,const  text& obj)
{

    float r = (float) obj.fillColor.r / 255;
    float g = (float) obj.fillColor.g / 255;
    float b = (float) obj.fillColor.b / 255;
    float a = (float) obj.fillColor.a / 255;

    cairo_set_font_size(cr, obj.size);
    cairo_move_to(cr, obj.position.x + obj.size, obj.position.y + obj.size);
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_show_text(cr, obj.label.c_str());

}



            //cairo_save(cairoCR_);

            vec2f position1 = points[i].position;
            vec2f position2 = points[i + 1].position;
            float radius = points[i].getArcRadius();

            vec2f center;
            if(radius < 0) center = circleCenter(position1, position2, std::abs(radius), direction::left);
            else center = circleCenter(position1, position2, radius, direction::right);
            //else if(radius == 0) cairo_line_to(cairoCR_, points[i + 1].position.x, points[i + 1].position.y);

            std::cout << center << std::endl;
            //double radius = sqrt(pow(position1.x - center.x, 2) + pow(position1.y - center.y, 2));

            double angle1 = 0;
            if(position1.y != center.y) angle1 = asin((position1.y - center.y) / points[i].getArcRadius());

            double angle2 = 0;
            if(position2.y != center.y) angle2 = asin((position2.y - center.y) / points[i].getArcRadius());

            if(radius < 0)
            {
                angle1 *= -1;
                angle2 *= -1;

                angle1 += M_PI / 2;
                angle2 += M_PI / 2;
            }

            if(points[i].getArcInverted())std::swap(angle1, angle2);

            std::cout << "a: " << angle1 << " " << angle2 << std::endl;

            //if(points[i].getArcInverted())cairo_arc_negative(cairoCR_, center.x, center.y, abs(radius), angle1, angle2);
            cairo_arc(cairoCR_, center.x, center.y, abs(radius), angle1, angle2);

            //cairo_restore(cairoCR_);
            */

}
