#pragma once

#include <ny/include.hpp>
#include <ny/app/eventHandler.hpp>

#include <nytl/vec.hpp>
#include <nytl/rect.hpp>

namespace ny
{

class Widget : public EventHandlerNode
{
public:
	virtual bool processEvent(const Event& event) override;
	virtual rect2ui extents() const;
	virtual void draw(DrawContext& dc);
};

}
