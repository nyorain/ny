#pragma once

#include <ny/include.hpp>
#include <ny/windowContext.hpp>
#include <nyutil/nonCopyable.hpp>

#include <vector>
#include <memory>

namespace ny
{

//defined backend constants
const unsigned int Wayland = 1;
const unsigned int X11 = 2;
const unsigned int Winapi = 3;

std::unique_ptr<windowContext> createWindowContext(window& win, const windowContextSettings& s = windowContextSettings());
std::unique_ptr<wc> createWC(window& win, const windowContextSettings& s = windowContextSettings());

class backend : public nonMoveable
{
protected:
    backend(unsigned int i);

    virtual std::unique_ptr<windowContext> createWindowContextImpl(window& win, const windowContextSettings& s) = 0;

public:
    virtual bool isAvailable() const = 0;

    virtual std::unique_ptr<appContext> createAppContext() = 0;
    std::unique_ptr<windowContext> createWindowContext(window& win, const windowContextSettings& s = windowContextSettings());

    virtual bool hasNativeHandling() const { return 0; };
    virtual bool hasNativeDecoration() const { return 0; };

    virtual bool hasCustomHandling() const { return 0; };
    virtual bool hasCustomDecoration() const { return 0; };

    const unsigned int id;

    bool operator==(const backend& other) const { return (other.id == id); }
    bool operator==(const unsigned int& otherID) const { return (otherID == id); }
    bool operator!=(const backend& other) const { return (other.id != id); }
    bool operator!=(const unsigned int& otherID) { return (otherID != id); }
};

}
