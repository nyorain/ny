#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/gl/egl.hpp>

namespace ny
{

class x11EGLDrawContext : public eglDrawContext
{
public:
    x11EGLDrawContext(const x11WindowContext& wc);
    virtual ~x11EGLDrawContext();
};

}
