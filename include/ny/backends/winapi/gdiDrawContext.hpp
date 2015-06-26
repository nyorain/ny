#pragma once

#include <ny/include.hpp"
#include <ny/graphics/drawContext.hpp"

#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

namespace ny
{

class winapiWindowContext;

class gdiDrawContext : public drawContext<2, 2>
{

friend winapiWindowContext;

protected:
    bool m_painting;

    HDC m_hdc;
    PAINTSTRUCT m_ps;
    Graphics* m_graphics;

    winapiWindowContext* m_windowContext;

public:
    gdiDrawContext(winapiWindowContext* wc);

    virtual void resize(vec2ui size){};
    virtual void clear(color col = color::none);

    virtual void mask(const shape2& obj){};
	virtual void fill(color col){};
	virtual void outline(color col){};

    virtual std::vector<rectangle> getClip(){};
    virtual void clip(const std::vector<rectangle>& clipVec){};
	virtual void resetClip(){};

    //specific
    void beginDraw();
    void finishDraw();

    HDC getHDC() const { return m_hdc; }
    PAINTSTRUCT getPS() const { return m_ps; }

    Graphics& getGraphics() const { return *m_graphics; }
};

}
