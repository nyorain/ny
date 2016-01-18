#pragma once

#include <ny/include.hpp>

#include <nytl/triangle.hpp>
#include <nytl/vec.hpp>

#include <vector>

namespace ny
{

//todo: single steps of algorithm, different algorithms, complete interface & class
std::vector<triangle2f> 
triangulate(const std::vector<vec2f>& pth);

}
