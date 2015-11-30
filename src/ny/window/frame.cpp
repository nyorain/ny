#include <ny/frame.hpp>

namespace ny
{

frame::frame(vec2i position, vec2ui size, const std::string& title, const windowContextSettings& settings) : toplevelWindow(position, size, title, settings)
{

}

frame::~frame()
{

}

}
