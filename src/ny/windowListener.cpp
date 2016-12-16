#include <ny/windowListener.hpp>
#include <ny/windowContext.hpp>
#include <ny/dataExchange.hpp>

//Those definitions are here, so we don't get unused parameter warnings in the header
//because we give them names

namespace ny
{

WindowListener& WindowListener::defaultInstance()
{
	static WindowListener instance;
	return instance;
}

WindowListener::WindowListener(WindowContext& wc)
{
	wc.listener(*this);
}

void WindowListener::dndEnter(DataOffer&, const EventData*)
{

}
DataFormat WindowListener::dndMove(nytl::Vec2i, DataOffer&, const EventData*)
{
	return DataFormat::none;
}
void WindowListener::dndLeave(DataOffer&, const EventData*)
{

}
void WindowListener::dndDrop(nytl::Vec2i, std::unique_ptr<DataOffer>, const EventData*)
{

}
void WindowListener::draw(const EventData*)
{

}
void WindowListener::close(const EventData*)
{

}
void WindowListener::position(nytl::Vec2i, const EventData*)
{

}
void WindowListener::resize(nytl::Vec2ui, const EventData*)
{

}
void WindowListener::state(bool, ToplevelState, const EventData*)
{

}
void WindowListener::key(bool, Keycode, const std::string&, const EventData*)
{

}
void WindowListener::focus(bool, const EventData*)
{

}
void WindowListener::mouseButton(bool, MouseButton, const EventData*)
{

}
void WindowListener::mouseMove(nytl::Vec2i, const EventData*)
{

}
void WindowListener::mouseWheel(float, const EventData*)
{

}
void WindowListener::mouseCross(bool, const EventData*)
{

}

}
