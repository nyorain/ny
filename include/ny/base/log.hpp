#pragma once

#include <ny/include.hpp>
#include <nytl/log.hpp>

namespace ny
{

//objects
logger& warningLogger();
logger& logLogger();
logger& errorLogger();
logger& debugLogger();

//functions
template<typename... Args>
inline void sendWarning(Args&&... args)
{
	warningLogger()(std::forward<Args>(args)...);
}

template<typename... Args>
inline void sendLog(Args&&... args)
{
	logLogger()(std::forward<Args>(args)...);
}

template<typename... Args>
inline void sendError(Args&&... args)
{
	errorLogger()(std::forward<Args>(args)...);
}

template<typename... Args>
inline void sendDebug(Args&&... args)
{
#ifndef NDEBUG
	debugLogger()(std::forward<Args>(args)...);
#else
	unused(args...);
#endif
}

}
