#pragma once

#include <ny/include.hpp>
#include <nytl/callback.hpp>

#include <map>
#include <functional>

namespace ny
{

class WidgetStyle
{
protected:
	static std::map<std::string, WidgetStyle> styles_;

public:
	static void style(const std::string& name, WidgetStyle& style);
	static WidgetStyle& style(const std::string& name);

public:
	callback<void(Widget& widget, DrawContext& dc)> onDraw;
};

}
