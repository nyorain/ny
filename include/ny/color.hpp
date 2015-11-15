#pragma once

#include <ny/include.hpp>
#include <nytl/vec.hpp>

#include <vector>

namespace ny
{

//TODO_____________


class colorBase
{

};

class color : public colorBase
{
public:
    color(unsigned char rx = 0, unsigned char gx = 0, unsigned char bx = 0, unsigned char ax = 255);

    unsigned int toInt();
    void normalized(float& pr, float& pg, float& pb, float& pa) const;
    void normalized(float& pr, float& pg, float& pb) const;

    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

    vec4uc rgba() const { return vec4uc(r,g,b,a); }
    vec3uc rgb() const { return vec3uc(r,g,a); }

    vec4f rgbaNorm() const { return vec4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f); }
    vec3f rgbNorm() const { return vec3f(r / 255.f, g / 255.f, b / 255.f); }

public:
    const static color red;
    const static color green;
    const static color blue;
    const static color white;
    const static color black;
    const static color none;
};

template<size_t dim> class gradient
{
protected:
    struct point
    {
        vec<dim, float> pos;
        color col;
    };

    std::vector<point> points_;

public:
    void addPoint(const vec<dim, float>& pos, const color& col) {};
    color getColorAt(const vec<dim, float>& pos) { return color(); };

    template<size_t odim> operator gradient<odim>()
    {
        gradient<odim> ret;
        for(auto& p : points_)
            ret.addPoint(p.pos, p.col);
        return ret;
    }
};

class brush1
{
protected:
    //some union stuff

public:
    brush1(const color& col) {}
    brush1(const image& img, vec2i position, vec2ui size, int mode) {} //mode like gl mode. stretch, fill etc
    brush1(const gradient<2>& grad) {}
    //customBrush class stuff?

    //some read stuff
};

class pen1
{
protected:
    float width_;
    //draw style
    //union

public:
    pen1(const color& col) {}
    pen1(const image& img, vec2i position, vec2ui size, int mode) {} //mode like gl mode. stretch, fill etc
    pen1(const gradient<2>& grad) {}
};

}
