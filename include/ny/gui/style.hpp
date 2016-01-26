#pragma once

#include <ny/include.hpp>
#include <nytl/callback.hpp>

#include <map>
#include <functional>

namespace ny
{

//
class BasicWidgetDrawer
{
	virtual void basic_draw(Widget& widget, DrawContext& dc) = 0;
};

//
template<typename W>
class WidgetDrawer : public BasicWidgetDrawer
{
public:
	virtual void draw(W&, DrawContext& dc) = 0; 

	virtual void basic_draw(Widget& widget, DrawContext& dc) override final
	{
		draw(reinterpret_cast<W&>(widget), dc);
	}
};

//
class WidgetStyle
{
protected:
	WidgetStyle(); //init default styles
	std::map<std::string, std::unique_ptr<BasicWidgetDrawer>> drawer_;

public:
	static WidgetStyle& instance()
	{
		static WidgetStyle instance_;
		return instance_;
	}
	
	template<typename W>
	static bool draw(W& widget, DrawContext& dc)
	{
		auto* drawer = instance().drawer<W>(widget.widgetClass());
		if(!drawer) return false;

		drawer->draw(widget, dc);
		return 1;
	}
	
public:
	template<typename WD>
	void drawer(const std::string& name, std::unique_ptr<WD>&& drawr)
	{
		drawer_[name] = std::move(drawr);
	}

	template<typename W>
	WidgetDrawer<W>* drawer(const std::string& name)
	{
		auto it = drawer_.find(name);
		if(it == drawer_.cend()) return nullptr;

		return reinterpret_cast<WidgetDrawer<W>*>(it->second.get());
	}
};

}
