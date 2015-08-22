#pragma once

#include <ny/include.hpp>
#include <nyutil/vec.hpp>

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
};

class brush1
{
protected:
    //some union stuff

public:
    brush1(const color& col) {}
    brush1(const image& img, vec2i position, vec2ui size, int mode) {}
    brush1(const gradient<2>& grad) {}
    //customBrush class stuff?

    //some read stuff
};

}
