#pragma once

#include <ny/include.hpp>

#include <nytl/triangle.hpp>
#include <nytl/vec.hpp>

#include <vector>

namespace ny
{

//todo: single steps of algorithm, different algorithms, complete interface & class
std::vector<Triangle2f> 
triangulate(const std::vector<Vec2f>& pth);

}
