#pragma once

#include <ny/include.hpp>
#include <cstdint>

namespace ny
{

namespace windowHints
{
    //toplevel specific
    constexpr unsigned long move = (1L << 1);
    constexpr unsigned long close = (1L << 2);
    constexpr unsigned long maximize = (1L << 3);
    constexpr unsigned long minimize = (1L << 4);
    constexpr unsigned long resize = (1L << 5);
    constexpr unsigned long customDecorated = (1L << 7);
    constexpr unsigned long acceptDrop = (1L << 10);
    constexpr unsigned long alwaysOnTop = (1L << 11);
    constexpr unsigned long showInTaskbar = (1L << 12);
};

namespace nativeWidgetType
{
    constexpr unsigned int nativeButton = 1;
    constexpr unsigned int nativeTextfield = 2;
    constexpr unsigned int nativeText = 3;
    constexpr unsigned int nativeCheckbox = 4;
    constexpr unsigned int nativeMenuBar = 5;
    constexpr unsigned int nativeToolbar = 6;
    constexpr unsigned int nativeProgressbar = 7;
    constexpr unsigned int nativeDialog = 8;
};

//windowEdge
enum class WindowEdge : unsigned char
{
    top,
    right,
    bottom,
    left,
    topLeft,
    topRight,
    bottomLeft,
    bottomRight,

    unknown
};

//virtuality
enum class Preference : unsigned char
{
    must,
    should,
    dontCare,
    shouldNot,
    mustNot
};

//toplevelState
enum class ToplevelState : unsigned char
{
    unknown = 0,

    maximized,
    minimized,
    fullscreen,
    normal
};


}
