#pragma once

#include <ny/include.hpp>

namespace ny
{

namespace windowHints
{
    //toplevel specific
    constexpr unsigned long Move = (1L << 1);
    constexpr unsigned long Close = (1L << 2);
    constexpr unsigned long Maximize = (1L << 3);
    constexpr unsigned long Minimize = (1L << 4);
    constexpr unsigned long Resize = (1L << 5);

    constexpr unsigned long nativeDecoration = (1L << 6);
    constexpr unsigned long customTitlebar = (1L << 7);
    constexpr unsigned long customBorder = (1L << 8);
    constexpr unsigned long customShadow = (1L << 9);

    constexpr unsigned long AcceptDrop = (1L << 10);

    constexpr unsigned long AlwaysOnTop = (1L << 11);
    constexpr unsigned long ShowInTaskbar = (1L << 12);
};

namespace nativeWindow
{
    constexpr unsigned int NativeButton = 1;
    constexpr unsigned int NativeTextfield = 2;
    constexpr unsigned int NativeText = 3;
    constexpr unsigned int NativeCheckbox = 4;
    constexpr unsigned int NativeMenuBar = 5;
    constexpr unsigned int NativeToolbar = 6;
    constexpr unsigned int NativeProgressbar = 7;
    constexpr unsigned int NativeDialog = 8;
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

//windowSettings
class windowContextSettings
{
public:
    virtual ~windowContextSettings();

    unsigned long hints; //backend-specific e.g. x11::TypeInputOnly, usually set by the window class
    unsigned int nativeWindow; //e.g. nativeWindow::button, usually set by the window class

    preference virtualPref = preference::DontCare;
    preference glPref = preference::DontCare;
    bool initShown = 1;
    toplevelState initState = toplevelState::Normal; //only used if window is toplevel window
};

}
