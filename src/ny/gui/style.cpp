#include <ny/gui/style.hpp>
#include <ny/gui/button.hpp>

#include <nytl/make_unique.hpp>

namespace ny
{

WidgetStyle::WidgetStyle()
{
	drawer("ny::Button", make_unique<Button::DefaultDrawer>());	
}

}
