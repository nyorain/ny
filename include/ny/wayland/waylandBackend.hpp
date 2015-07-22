#pragma once

#include <ny/backend.hpp>
#include <ny/windowContext.hpp>

namespace ny
{

class waylandBackend : public backend
{
protected:
    static const waylandBackend object;

public:
    waylandBackend();

    virtual bool isAvailable() const;

    virtual appContext* createAppContext();
    virtual windowContext* createWindowContext(window& win, const windowContextSettings& s = windowContextSettings());

    virtual bool hasNativeHandling() const { return 0; };
    virtual bool hasNativeDecoration() const { return 0; };

    virtual bool hasCustomHandling() const { return 1; };
    virtual bool hasCustomDecoration() const { return 1; };
};

}
