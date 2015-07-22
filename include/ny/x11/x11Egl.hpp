#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/gl/egl.hpp>

namespace ny
{

class x11EGLContext : public eglContext
{
public:
    x11EGLContext(const x11WindowContext& wc);
    virtual ~x11EGLContext();
};

}
