#pragma once

#include <ny/backends/x11/x11Include.hpp>
#include <ny/backends/backend.hpp>

namespace ny
{

class x11Backend : public backend
{
protected:
    static const x11Backend object;

public:
    x11Backend();

    virtual bool isAvailable() const;

    virtual appContext* createAppContext();
    virtual toplevelWindowContext* createToplevelWindowContext(toplevelWindow& win, const windowContextSettings& s = windowContextSettings());
    virtual childWindowContext* createChildWindowContext(childWindow& win, const windowContextSettings& s = windowContextSettings());
    //virtual windowContext* createCustomWindowContext(window* win, unsigned int hints = 0, const windowContextSettings& s = windowContextSettings());

    virtual bool hasNativeHandling() const { return 1; };
    virtual bool hasNativeDecoration() const { return 1; };

    virtual bool hasCustomHandling() const { return 1; };
    virtual bool hasCustomDecoration() const { return 1; };
};

}
