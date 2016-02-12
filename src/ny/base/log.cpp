#include <ny/base/log.hpp>

namespace ny
{


Logger& warningLogger()
{
	static Logger object("", "warning", std::cout);
	return object;
}

Logger& logLogger()
{
	static Logger object("", "log", std::clog);
	return object;
}

Logger& errorLogger()
{
	static Logger object("", "error", std::cerr);
	return object;
}

Logger& debugLogger()
{
#ifndef NDEBUG
	static Logger object("", "error", std::cerr);
	return object;
#else
	static Logger object("", "error"); //invalid object - no stream
	return object;
#endif
}

}
