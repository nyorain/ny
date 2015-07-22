#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/gl/glContext.hpp>

namespace ny
{

struct glxc;

////////////////////////////
class glxContext : public glContext
{
protected:
    const x11WindowContext& wc_;
    glxc* glxContext_ = nullptr;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

public:
    glxContext(const x11WindowContext& wc);
    ~glxContext();
};

}
