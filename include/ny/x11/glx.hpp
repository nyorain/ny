#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/gl/glDrawContext.hpp>

namespace ny
{

struct glxc;

////////////////////////////
class glxDrawContext : public glDrawContext
{
protected:
    const x11WindowContext& wc_;
    glxc* glxContext_ = nullptr;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;
    virtual bool swapBuffers() override;

public:
    glxDrawContext(const x11WindowContext& wc);
    ~glxDrawContext();
};

}
