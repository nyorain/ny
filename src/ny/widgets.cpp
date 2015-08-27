#include <ny/widgets.hpp>
#include <ny/shape.hpp>
#include <ny/drawContext.hpp>

namespace ny
{

//button
button::button(window& parent, vec2ui position, vec2ui size, const std::string& label, const windowContextSettings& settings) : widget(parent, position, size, settings), label_(label)
{
    drawCallback_.add([](window& w, drawContext& dc){ static_cast<button&>(w).cbDraw(dc); });
    mouseButtonCallback_.add([](window& w, const mouseButtonEvent& ev){ static_cast<button&>(w).cbButton(ev); });
    focusCallback_.add([](window& w, const focusEvent& ev){ static_cast<button&>(w).cbFocus(ev); });
    mouseCrossCallback_.add([](window& w, const mouseCrossEvent& ev){ static_cast<button&>(w).cbCross(ev); });
}

void button::cbDraw(drawContext& dc)
{
    color ccol = color::white;
    if(mouseOver_) ccol = color(200, 200, 200);
    if(pressed_) ccol = color(150, 150, 150);
    dc.clear(ccol);

    text txt((getSize() / 2) - vec2ui(0, 7), label_, 14);
    txt.setBound(textBound::center);

    dc.mask(txt);
    dc.fill(color::green);
}

void button::cbButton(const mouseButtonEvent& ev)
{
    if(ev.button == mouse::left )
    {
        if(ev.state == pressState::pressed)
        {
            pressed_ = 1;
            refresh();
        }
        else if(ev.state == pressState::released)
        {
            if(pressed_)
                clickCallback_(*this);

            pressed_ = 0;
            refresh();
        }
    }
}

void button::cbFocus(const focusEvent& ev)
{
    refresh();
}

void button::cbCross(const mouseCrossEvent& ev)
{
    pressed_ = 0; //todo, register some handler to further receive events for button release
    refresh();
}

//textfield

}
