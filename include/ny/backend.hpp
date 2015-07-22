#pragma once

#include <ny/include.hpp>
#include <ny/windowContext.hpp>

#include <vector>

namespace ny
{

//defined backend constants
const unsigned int Wayland = 1;
const unsigned int X11 = 2;
const unsigned int Winapi = 3;

windowContext* createWindowContext(window& win, const windowContextSettings& s = windowContextSettings());
wc* createWC(window& win, const windowContextSettings& s = windowContextSettings());

class backend
{
protected:
    backend(unsigned int i);

public:
    virtual bool isAvailable() const = 0;

    virtual appContext* createAppContext() = 0;
    virtual windowContext* createWindowContext(window& win, const windowContextSettings& s = windowContextSettings()) = 0;

    virtual bool hasNativeHandling() const { return 0; };
    virtual bool hasNativeDecoration() const { return 0; };

    virtual bool hasCustomHandling() const { return 0; };
    virtual bool hasCustomDecoration() const { return 0; };

    const unsigned int id;


    bool operator==(const backend& other)
    {
        return (other.id == id);
    }

    bool operator==(const unsigned int& otherID)
    {
        return (otherID == id);
    }

    bool operator!=(const backend& other)
    {
        return (other.id != id);
    }

    bool operator!=(const unsigned int& otherID)
    {
        return (otherID != id);
    }
};

}
