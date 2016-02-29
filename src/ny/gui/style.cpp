#include <ny/gui/style.hpp>
#include <ny/gui/button.hpp>

namespace ny
{

WidgetStyle::WidgetStyle()
{
	drawer("ny::Button", std::make_unique<Button::DefaultDrawer>());	
}

}
