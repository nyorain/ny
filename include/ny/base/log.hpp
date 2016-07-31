#pragma once

#include <ny/include.hpp>
#include <nytl/log.hpp>

namespace ny
{

//objects
Logger& warningLogger();
Logger& logLogger();
Logger& errorLogger();
Logger& debugLogger();

//functions
template<typename... Args>
inline void warning(Args&&... args)
{
	warningLogger()(std::forward<Args>(args)...);
}

template<typename... Args>
inline void log(Args&&... args)
{
	logLogger()(std::forward<Args>(args)...);
}

template<typename... Args>
inline void error(Args&&... args)
{
	errorLogger()(std::forward<Args>(args)...);
}

template<typename... Args>
inline void debug(Args&&... args)
{
#ifndef NDEBUG
	debugLogger()(std::forward<Args>(args)...);
#else
	unused(args...);
#endif
}

}
