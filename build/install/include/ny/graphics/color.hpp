#pragma once

#include "include.hpp"

namespace ny
{
class colorBase
{

};

class color : public colorBase
{
public:
    color(unsigned char rx = 0, unsigned char gx = 0, unsigned char bx = 0, unsigned char ax = 255);

    unsigned int toInt();
    void normalized(float& pr, float& pg, float& pb, float& pa);
    void normalized(float& pr, float& pg, float& pb);

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

}
