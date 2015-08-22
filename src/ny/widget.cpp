#include <ny/widget.hpp>

namespace ny
{

widget::widget(window& parent, vec2i position, vec2ui size, const windowContextSettings& settings) : childWindow(parent, position, size, settings)
{
}

}
