#pragma once

#include <ny/include.hpp>
#include <ny/backend/windowContext.hpp>
#include <ny/window/windowDefs.hpp>
#include <nytl/nonCopyable.hpp>

#include <vector>
#include <memory>
#include <string>

namespace ny
{

class Backend : public nonMoveable
{
public:
	using AppContextPtr = std::unique_ptr<AppContext>;
	using WindowContextPtr = std::unique_ptr<WindowContext>;

	static std::vector<Backend*> backends();

protected:
	static std::vector<Backend*> backendsFunc(Backend* reg = nullptr, bool remove = 0);

protected:
	Backend();
	~Backend();

public:
    virtual bool available() const = 0;

    virtual AppContextPtr createAppContext() = 0;
	virtual WindowContextPtr createWindowContext(Window& win, 
			const WindowContextSettings& s = {}) = 0;

	virtual std::string name() const = 0;
};

}
