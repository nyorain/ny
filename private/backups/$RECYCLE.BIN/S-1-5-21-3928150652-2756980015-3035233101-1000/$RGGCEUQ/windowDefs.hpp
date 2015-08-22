#pragma once

#include <ny/include.hpp>

namespace ny
{

namespace windowHints
{
    const unsigned long GL = (1L << 1);
    const unsigned long Toplevel = (1L << 2);
    const unsigned long Child = (1L << 3);

    //toplevel specific
    const unsigned long Move = (1L << 1);
    const unsigned long Close = (1L << 2);
    const unsigned long Maximize = (1L << 3);
    const unsigned long Minimize = (1L << 4);
    const unsigned long Resize = (1L << 5);
    //const unsigned long Virtual (1L << 9); /no real window. must be childWindow, is only drawn on parent and has no real backend handle.

    const unsigned long AcceptDrop = (1L << 6);

    //will only work if possible
    const unsigned long CustomDecorated = (1L << 9);
    const unsigned long CustomResized = (1L << 10);
    const unsigned long CustomMoved = (1L << 11);

    //not available on all
    const unsigned long AlwaysOnTop = (1L << 17);
    const unsigned long ShowInTaskbar = (1L << 18); //ShowInPager e.g. for x11?? needed?


    //winapi and gtk backend: how to handler native/ external drawn/ created widgets?
    const unsigned long NativeButton = (1L << 21);
    const unsigned long NativeTextfield = (1L << 22);
    const unsigned long NativeText = (1L << 23);
    const unsigned long NativeCheckbox = (1L << 24);
    const unsigned long NativeMenuBar = (1L << 25);
    const unsigned long NativeToolbar = (1L << 26);
    const unsigned long NativeProgressbar = (1L << 27);
    const unsigned long NativeDialog = (1L << 28);

};

//windowEdge
enum class windowEdge : unsigned char
{
    Top,
    Right,
    Bottom,
    Left,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,

    Unknown
};

//virtuality
enum class preference : unsigned char
{
    Must,
    Should,
    DontCare,
    ShouldNot,
    MustNot
};

//toplevelState
enum class toplevelState : unsigned char
{
    Unknown = 0,

    Maximized,
    Minimized,
    Fullscreen,
    Normal,
    Modal
};

//wcSettings
class windowContextSettings
{
public:
    virtual ~windowContextSettings();

    unsigned long hints; //backend-specific e.g. x11::TypeInputOnly
    preference virtualPref = preference::DontCare;
    preference glPref = preference::DontCare;
};

}
