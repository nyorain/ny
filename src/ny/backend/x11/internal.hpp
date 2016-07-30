#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

namespace ny
{

//dummy used for unnamed struct typedef declaration
struct EwmhConnection : public xcb_ewmh_connection_t {};

}
