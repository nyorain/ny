#pragma once

#include <ny/include.hpp>
#include <ny/drawContext.hpp>

#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

namespace ny
{

struct gdiFont
{

};

class winapiWindowContext;

class gdiDrawContext : public drawContext
{
friend winapiWindowContext;

protected:
    bool painting_ = 0;

    HDC hdc_;
    PAINTSTRUCT ps_;
    Graphics* graphics_ = nullptr;

    winapiWindowContext& wc_;

public:
    gdiDrawContext(winapiWindowContext& wc);

    virtual void clear(color col = color::none) override;

	virtual void mask(const customPath& obj) override {};
	virtual void mask(const text& obj) override {};
	virtual void resetMask() override {};

	virtual void fillPreserve(const brush& col) override {};
	virtual void strokePreserve(const pen& col) override {};

    virtual rect2f getClip() override { return rect2f(); };
    virtual void clip(const rect2f& obj) override {};
	virtual void resetClip() override {};

    //specific
    void setSize(vec2ui size);

    void beginDraw();
    void finishDraw();

    HDC getHDC() const { return hdc_; }
    PAINTSTRUCT getPS() const { return ps_; }
    Graphics& getGraphics() const { return *graphics_; }
};

}
