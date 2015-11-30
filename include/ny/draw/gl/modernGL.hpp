#pragma once

#include <ny/include.hpp>
#include <ny/gl/glDrawContext.hpp>

namespace ny
{

//modern
class modernGLDrawImpl : public glDrawImpl
{
public:
    virtual void clear(color col) override;
    virtual void fill(const mask& m, const brush& b) override;
    virtual void stroke(const mask& m, const pen& b) override;
};

}
