#include <ny/base/log.hpp>

namespace ny
{


logger& warningLogger()
{
	static logger object("", "warning", std::cout);
	return object;
}

logger& logLogger()
{
	static logger object("", "log", std::clog);
	return object;
}

logger& errorLogger()
{
	static logger object("", "error", std::cerr);
	return object;
}

logger& debugLogger()
{
#ifndef NDEBUG
	static logger object("", "error", std::cerr);
	return object;
#else
	static logger object("", "error"); //invalid object - no stream
	return object;
#endif
}

}
