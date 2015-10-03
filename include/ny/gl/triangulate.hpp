#pragma once

#include <ny/include.hpp>
#include <ny/shape.hpp>
#include <nyutil/triangle.hpp>

#include <vector>

namespace ny
{

//todo: single steps of algorithm, different algorithms, complete interface & class
std::vector<triangle2> triangulate(float* points, std::size_t size);

}