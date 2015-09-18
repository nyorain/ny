#pragma once

#include <ny/include.hpp>
#include <ny/backend.hpp>

#include <memory>

namespace ny
{

class winapiBackend : public backend
{
protected:
    static winapiBackend object;

    virtual std::unique_ptr<windowContext> createWindowContextImpl(window& win, const windowContextSettings& settings = windowContextSettings()) override;

public:
    winapiBackend();

    virtual bool isAvailable() const { return 1; } //todo: can this be implemented? should always be available under windows

    virtual std::unique_ptr<appContext> createAppContext() override;

    virtual bool hasNativeHandling() const { return 1; };
    virtual bool hasNativeDecoration() const { return 1; };

    virtual bool hasCustomHandling() const { return 1; };
    virtual bool hasCustomDecoration() const { return 1; };
};

}
