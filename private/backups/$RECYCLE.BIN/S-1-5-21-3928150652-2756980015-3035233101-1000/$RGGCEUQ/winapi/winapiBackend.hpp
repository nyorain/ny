#pragma once

#include <ny/include.hpp>
#include <ny/backend.hpp>

namespace ny
{

class winapiBackend : public backend
{
protected:
    static const winapiBackend object;

public:
    winapiBackend();

    virtual bool isAvailable() const { return 1; } //todo: implement

    virtual appContext* createAppContext();

    virtual windowContext* createWindowContext(window& win, const windowContextSettings& settings = windowContextSettings());

    virtual bool hasNativeHandling() const { return 1; };
    virtual bool hasNativeDecoration() const { return 1; };

    virtual bool hasCustomHandling() const { return 1; };
    virtual bool hasCustomDecoration() const { return 1; };
};

}
