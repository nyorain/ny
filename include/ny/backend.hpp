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

toplevelWindowContext* createToplevelWindowContext(toplevelWindow& win, unsigned int type);
childWindowContext* createChildWindowContext(childWindow& win, unsigned int type);
windowContext* createCustomWindowContext(window& win, unsigned int type);

toplevelWC* createToplevelWC(toplevelWindow& win, unsigned int type);
childWC* createChildWC(childWindow& win, unsigned int type);
wc* createCustomWC(window& win, unsigned int type);

class backend
{
protected:
    backend(unsigned int i);

public:
    virtual bool isAvailable() const = 0;

    virtual appContext* createAppContext() = 0;

    virtual toplevelWindowContext* createToplevelWindowContext(toplevelWindow& win, const windowContextSettings& s = windowContextSettings()) = 0;
    virtual childWindowContext* createChildWindowContext(childWindow& win, const windowContextSettings& s = windowContextSettings()) = 0;
    virtual windowContext* createCustomWindowContext(window& win, const windowContextSettings& s = windowContextSettings()) { return nullptr; };

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

    //dummys
    ac* createAC(){ return createAppContext(); };
    toplevelWC* createToplevelWC(toplevelWindow& win, const windowContextSettings& s = windowContextSettings()){ return createToplevelWindowContext(win, s); };
    childWC* createChildWC(childWindow& win, const windowContextSettings& s = windowContextSettings()){ return createChildWindowContext(win, s); };
    wc* createCustomWC(window& win, const windowContextSettings& s = windowContextSettings()){ return createCustomWindowContext(win, s); };
};

}
