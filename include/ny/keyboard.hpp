#pragma once

#include <ny/include.hpp>

#include <bitset>

namespace ny
{


class keyboard
{
protected:
    static std::bitset<255> states;
    static bool capsLockState;

public:
    enum key
    {
        /* none    */   none = -1,
        /* chars   */   a = 0, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
        /* numbers */   num1, num2, num3, num4, num5, num6, num7, num8, num9, num0, numpad1, numpad2, numpad3, numpad4, numpad5, numpad6, numpad7, numpad8, numpad9, numpad0,
        /* func    */   f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
        /* else    */   play, stop, next, previous, escape, comma, dot, sharp, plus, minus, tab, leftctrl, rightctrl, leftsuper, rightsuper, leftshift, rightshift,
                        space, enter, backspace, del, end, insert, pageUp, pageDown,  home,  back,  left, up, down, right, volumeup, volumedown, leftalt, rightalt, capsLock
    };

    static char toChar(key);
    static bool isKeyPressed(key);

    static void pressKey(key);
    static void releaseKey(key);

    static bool shiftActive();
    static bool altActive();
    static bool ctrlActive();
};


}
