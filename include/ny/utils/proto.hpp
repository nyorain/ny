#pragma once

namespace ny
{
	
	template<size_t rows, size_t cols, class prec> class mat;
	template<size_t dim, class T> class vec;
	template<size_t dim, class T> class refVec;
	template<size_t dim, class prec> class rect;

	template < class > class callback;
	class connection;
	class callbackBase;

	class timeDuration;
	class timePoint;
	class timer;

	class region;
	class threadpool;
	class task;
	class threadSafeObj;
	class nonCopyable;
}
