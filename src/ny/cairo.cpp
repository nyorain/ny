#include <ny/cairo.hpp>

#include <ny/shape.hpp>
#include <ny/error.hpp>
#include <ny/surface.hpp>

#include <iostream>
#include <cmath>

namespace ny
{

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


//cairoDC////////////////////////////////////////////////////////////////////////////////////77
cairoDrawContext::cairoDrawContext(surface& surf) : drawContext(surf), cairoCR_(nullptr)
{
}

cairoDrawContext::cairoDrawContext(surface& surf, cairo_surface_t& cairoSurface) : drawContext(surf), cairoSurface_(cairo_surface_reference(&cairoSurface)), cairoCR_(cairo_create(&cairoSurface))
{
}

cairoDrawContext::~cairoDrawContext()
{
    if(cairoCR_) cairo_destroy(cairoCR_);
    if(cairoSurface_) cairo_surface_destroy(cairoSurface_); //can be done, in constructor reference is called
}

void cairoDrawContext::clear(color col)
{
    if(!cairoCR_)
    {
        sendWarning("drawing with uninitialized cairoDC");
        return;
    }

    float r, g, b, a = 0;
    col.normalized(r, g, b, a);

    cairo_set_source_rgba(cairoCR_, r, g, b, a);
    cairo_paint(cairoCR_);
}

rect2f cairoDrawContext::getClip()
{
    rect2f ret;

    if(!cairoCR_)
    {
        sendWarning("drawing with uninitialized cairoDC");
        return ret;
    }

    cairo_rectangle_list_t* recList = cairo_copy_clip_rectangle_list(cairoCR_);

    cairo_rectangle_t& r = recList->rectangles[0];
    ret.position = vec2f(r.x, r.y);
    ret.size =  vec2f(r.width, r.height);

    return ret;
}

void cairoDrawContext::clip(const rect2f& obj)
{
    if(!cairoCR_)
    {
        sendWarning("drawing with uninitialized cairoDC");
        return;
    }

    cairo_rectangle(cairoCR_, obj.left(), obj.top(), obj.width(), obj.height());
    cairo_clip(cairoCR_);
}

void cairoDrawContext::resetClip()
{
    if(!cairoCR_)
    {
        sendWarning("drawing with uninitialized cairoDC");
        return;
    }

    cairo_reset_clip(cairoCR_);
}


void cairoDrawContext::mask(const text& obj)
{
    if(!cairoCR_)
    {
        sendWarning("drawing with uninitialized cairoDC");
        return;
    }

    if(!obj.getFont())
    {
        sendWarning("drawing text with no font");
        return;
    }

    cairo_set_font_face(cairoCR_, obj.getFont()->getCairoHandle(1)->handle);
    cairo_set_font_size(cairoCR_, obj.getSize());
    cairo_show_text(cairoCR_, obj.getString().c_str());
}

void cairoDrawContext::mask(const customPath& obj)
{
    if(!cairoCR_)
    {
        sendWarning("drawing with uninitialized cairoDC");
        return;
    }

    cairo_matrix_t cMatrix;
    cairo_get_matrix(cairoCR_, &cMatrix);

    //cairo_translate(cairoCR_, obj.getTranslation().x, obj.getTranslation().y);
    //cairo_scale(cairoCR_, obj.getScale().x, obj.getScale().y);
    //cairo_rotate(cairoCR_, obj.getRotation());

    cairo_new_path(cairoCR_);

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
        }

        //tangent
        else if(points[i].getDrawStyle() == drawStyle::bezier)
        {
            bezierData data = points[i].getBezierData();
            cairo_curve_to(cairoCR_, data.start.x, data.start.y, data.end.x, data.start.y, points[i].position.x, points[i].position.y);
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

    cairo_close_path(cairoCR_);
    cairo_set_matrix(cairoCR_, &cMatrix);
}

void cairoDrawContext::resetMask()
{
    //todo
}

void cairoDrawContext::outline(const brush& col)
{
    if(!cairoCR_)
    {
        sendWarning("drawing with uninitialized cairoDC");
        return;
    }

    float r, g, b, a = 0;
    col.normalized(r, g, b, a);

    cairo_set_source_rgba(cairoCR_, r, g, b, a);
    cairo_stroke(cairoCR_);
}

void cairoDrawContext::fill(const pen& col)
{
    if(!cairoCR_)
    {
        sendWarning("drawing with uninitialized cairoDC");
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

