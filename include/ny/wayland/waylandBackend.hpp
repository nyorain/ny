#pragma once

#include <ny/backend.hpp>
#include <ny/windowContext.hpp>

namespace ny
{

class waylandBackend : public backend
{
protected:
    static const waylandBackend object;

    virtual std::unique_ptr<windowContext> createWindowContextImpl(window& win, const windowContextSettings& s = windowContextSettings()) override;

public:
    waylandBackend();

    virtual bool isAvailable() const override;

    virtual std::unique_ptr<appContext> createAppContext() override;

    virtual bool hasNativeHandling() const override { return 0; };
    virtual bool hasNativeDecoration() const override { return 0; };

    virtual bool hasCustomHandling() const override { return 1; };
    virtual bool hasCustomDecoration() const override { return 1; };
};

}
