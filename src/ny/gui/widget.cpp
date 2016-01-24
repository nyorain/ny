#include <ny/gui/widget.hpp>
#include <ny/window/nativeWidget.hpp>

namespace ny
{

Widget::Widget(WidgetBase& parent) : hierachyNode<WidgetBase>(parent), nativeWidget_(nullptr)
{
}

Widget::~Widget() = default;

}
