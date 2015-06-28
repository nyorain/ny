#pragma once

#include <ny/include.hpp>
#include <ny/widget.hpp>

namespace ny
{

/*
/button////////////////////7
class button : public widget
{
protected:
    std::string m_label;
    bool m_pressed;
    bool m_mouseOver;

    virtual void evt_onClick();
    virtual void evt_onRelease();

    virtual void mouseButton(mouseButtonEvent* e);
    virtual void mouseFocus(mouseFocusEvent* e);
    virtual void mouseMoved(mouseMoveEvent* e);

public:
    button(window* parent, vec2ui size, vec2ui position, std::string label = "");

    virtual void draw(drawContext* dc);

    buttonCallback onClick;
};

/textfield////////////////////////////
class textfield : public widget
{
protected:
    std::string m_label;
    bool m_focused;

    virtual void evt_onFocus();
    virtual void evt_onEnter();
    virtual void evt_onChange();

    virtual void keyboardKey(keyEvent* e);
    virtual void mouseButton(mouseButtonEvent* e);

    int m_lastTime;
    bool m_showM;

public:
    textfield(window* parent, vec2ui size, vec2ui position, std::string label);

    virtual void draw(drawContext* dc);

    textFieldCallback onFocus;
    textFieldCallback onEnter;
    textFieldCallback onChange;
};

/panel//////////////////////
class panel : public widget
{
public:
    panel(window* parent, vec2ui size, vec2ui position);

    virtual void draw(drawContext* dc);
};
*/

}
