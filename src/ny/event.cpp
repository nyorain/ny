#include <ny/event.hpp>

namespace ny
{

event::event(unsigned int xtype): type(xtype), backend(0), data(nullptr)
{
}

event::~event()
{
    if(data)
        delete data;
}

}
