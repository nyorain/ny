#pragma once

#include <ny/include.hpp>
#include <ny/widget.hpp>

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

    std::unique_ptr<connection> onClick(std::function<void(button&)> func){ return clickCallback_.add(func); };

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

    std::unique_ptr<connection> onEdit(std::function<void(textfield&, const std::string&)> func){ return editCallback_.add(func); };
    std::unique_ptr<connection> onEnter(std::function<void(textfield&, const std::string&)> func){ return enterCallback_.add(func); };

    const std::string& getLabel() const { return label_; }
    void setLabel(const std::string& lbl) { label_ = lbl; refresh(); }

    //widget
    virtual std::string getWidgetName() const override { return "ny::textfield"; }
};

}
