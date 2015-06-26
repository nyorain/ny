#pragma once

#include "include.hpp"
#include "graphics/color.hpp"

namespace ny
{

class style
{
public:

    class button
    {
    public:
        static color background;
        static color foreground;
        static color focusedBackground;
        static color focusedForeground;
        static color borderColor;

        static float borderWidth;
        static float borderRadius;
    };

    class frame
    {
    public:
        static color background;
        static color foreground;
        static color focusedBackground;
        static color focusedForeground;
        static color borderColor;
        static float borderRadius;

        static float borderWidth;
    };

    class control
    {
    public:
        static color background;
        static color foreground;
        static color focusedBackground;
        static color focusedForeground;
        static color borderColor;

        static float borderWidth;
    };

    class headerBar
    {
    public:
        static color background;
        static color foreground;
        static color focusedBackground;
        static color focusedForeground;
        static color borderColor;

        static float borderWidth;
        static float borderRadius1;
        static float borderRadius2;
        static float borderRadius3;
        static float borderRadius4;
    };

};

}
