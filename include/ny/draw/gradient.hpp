#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/color.hpp>

#include <nytl/vec.hpp>
#include <nytl/sequence.hpp>

namespace ny
{

class ColorGradient : public nytl::sequence<1, float, Color>
{		
};


}
