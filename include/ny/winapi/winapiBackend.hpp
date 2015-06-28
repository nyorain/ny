#pragma once

#include <ny/include.hpp"
#include <ny/backend.hpp"

namespace ny
{

class winapiBackend : public backend
{
protected:
    static const winapiBackend object;
public:
    winapiBackend();

    virtual bool isAvailable() const { return 1; } /todo: implement

    virtual appContext* createAppContext();

    virtual toplevelWindowContext* createToplevelWindowContext(toplevelWindow* win, const windowContextSettings& settings = windowContextSettings());
    virtual childWindowContext* createChildWindowContext(childWindow* win, const windowContextSettings& settings = windowContextSettings());
};

}
