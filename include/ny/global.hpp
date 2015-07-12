#pragma once

#include <ny/ny.hpp>
#include <ny/util/callback.hpp>

namespace ny
{

connection& onError(std::function<void()> cb);
connection& onExit(std::function<void()> cb);

}
