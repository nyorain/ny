#pragma once

#include <ny/include.hpp>
#include <ny/widget.hpp>
#include <nyutil/time.hpp>

namespace ny
{

//button///////////////////////
class button : public widget
{
private:
    void cbButton(const mouseButtonEvent& ev);
    void cbFocus(const focusEvent& ev);
    void cbDraw(drawContext& dc);
    void cbCross(const mouseCrossEvent& ev);

protected:
    std::string label_ {};
    bool pressed_ {0};

    callback<void(button&)> clickCallback_ {};

public:
    button(window& parent, vec2ui position, vec2ui size, const std::string& label = "", const windowContextSettings& = windowContextSettings());

    template<typename F> connection onClick(F&& func){ return clickCallback_.add(func); };

    const std::string& getLabel() const { return label_; }
    void setLabel(const std::string& lbl) { label_ = lbl; refresh(); }

    //widget
    virtual std::string getWidgetName() const override { return "ny::button"; }
};

//textfield////////////////////////////
class textfield : public widget
{
private:
    void cbButton(const mouseButtonEvent& ev);
    void cbFocus(const focusEvent& ev);
    void cbKey(const keyEvent& key);
    void cbDraw(drawContext& dc);

protected:
    std::string label_ {};

    timePoint lastBlinkTime_ {};
    bool showCursor_ {};

    callback<void(textfield&, const std::string&)> editCallback_ {};
    callback<void(textfield&, const std::string&)> enterCallback_ {};

public:
    textfield(window* parent, vec2ui size, vec2ui position, std::string label, const windowContextSettings& = windowContextSettings());

    template<typename F> connection onEdit(F&& func){ return editCallback_.add(func); };
    template<typename F> connection onEnter(F&& func){ return enterCallback_.add(func); };

    const std::string& getLabel() const { return label_; }
    void setLabel(const std::string& lbl) { label_ = lbl; refresh(); }

    //widget
    virtual std::string getWidgetName() const override { return "ny::textfield"; }
};

}
