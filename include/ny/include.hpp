#pragma once

#include <ny/config.h>
#include <cstdlib>

namespace nytl
{

template<size_t dim, class T> class vec;
template<size_t dim, typename T> using refVec = vec<dim, T&>;
template<size_t dim, class prec> class rect;
template<size_t dim, class prec> class triangle;
template<size_t dim, class prec> class line;
template<size_t dim, typename prec> class transformable;
template < class > class callback;
template < class > class compatibleFunction;

class connection;
class callbackBase;

class timeDuration;
class timePoint;
class timer;

class nonCopyable;
}


namespace ny
{
using namespace nytl;
}

