#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/backend.hpp>

namespace ny
{

class x11Backend : public backend
{
protected:
    static const x11Backend object;

    virtual std::unique_ptr<windowContext> createWindowContextImpl(window& win, const windowContextSettings& s) override;

public:
    x11Backend();

    virtual bool isAvailable() const;

    virtual std::unique_ptr<appContext> createAppContext() override;

    virtual bool hasNativeHandling() const override { return 1; };
    virtual bool hasNativeDecoration() const override { return 1; };

    virtual bool hasCustomHandling() const override { return 1; };
    virtual bool hasCustomDecoration() const override { return 1; };
};

}
