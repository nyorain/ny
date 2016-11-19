// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <nytl/bits/tmpUtil.inl>

#include <memory>
#include <sstream>

namespace ny
{

///Outputs the given text over the given ostream.
///Just a wrapper function around the << ostream operator.
///If on windows, checks if the given ostream is associated with one of the standard
///output streams (stdout, stderr) and if so, outputs the text using winapi directly
///to allow correct utf8 output (which is usually messed up by stl implementations/winapi).
void writeStream(std::ostream& out, const std::string& text);

///Base class for outputting data.
///Abstract outputting data over various stream/output methods.
///Note that this extra layer of abstraction over e.g. stl streams are used by ny
///to enable utf8 unicode support on windows, create multiple logging object for
///different purposes (that may be e.g. redirected to files or muted by the application) and
///to allow for greater flexibility (e.g. allows the application to use custom stream
///implementations without having to redirect the standard stream objects).
///Still uses the << operator (in form of std::stringstream) to convert given arguments
///to string form.
class LoggerBase
{
public:
	LoggerBase() = default;
	virtual ~LoggerBase() = default;

	virtual void write(const std::string& text) const = 0;

	template<typename... Args>
	void output(Args&&... args)
	{
		std::stringstream sstream;
		(void) Expand{(sstream << args, 0)...};
		write(sstream.str());
	}

	template<typename... Args>
	void operator()(Args&&... args)
		{ output(std::forward<Args>(args)...); }
};

///LoggerBase implementation for some Stream object.
///To make this class work, there has to be a function writeStream(Stream&, const std::string&)
///that outputs the given text over the given Stream object.
///This is implemented as expected only for std::ostream objects by ny.
template <typename Stream>
class Logger : public LoggerBase
{
public:
	Stream* stream {};
	std::string logPrefix {};

public:
	Logger() = default;
	Logger(Stream& out, const std::string& logPrefix = "") : stream(&out), logPrefix(logPrefix) {}

	Logger(Logger& other) = default;
	Logger& operator=(Logger& other) = default;

	std::string prefix() const { return logPrefix; }
	void write(const std::string& text) const override
		{ if(stream) writeStream(*stream, logPrefix + text + "\n"); }
};

///Can be used to completely mute a stream, i.e. make it output nothing at all.
///Used in non-debug builds for the debug logger (although it the debug() function should
///additionally not use it to avoid some runtime overhead).
///E.g. to completely mute the logging stream:
///```
///ny::warningLogger(std::make_unique<ny::DummyStream>());
///```
class DummyStream : public LoggerBase
{
	void write(const std::string&) const override {}
};

std::unique_ptr<LoggerBase>& warningLogger();
std::unique_ptr<LoggerBase>& logLogger();
std::unique_ptr<LoggerBase>& errorLogger();
std::unique_ptr<LoggerBase>& debugLogger();

//functions
///Should be used to output critical errors. Could be e.g. done before throwing an exception
///to provide additional information.
///Is usually used by application if exiting the application due to an usage error or if
///e.g. some critical excpetion is thrown.
template<typename... Args>
inline void error(Args&&... args) { errorLogger()->output(std::forward<Args>(args)...); }

///Outputs a warning with the given arguments.
///Warning should be outputted if some operation cannot be done correctly, but this
///failure is not considered critical so no exception is thrown.
template<typename... Args>
inline void warning(Args&&... args) { warningLogger()->output(std::forward<Args>(args)...); }

///This should be used to debug additional information that can be useful for the user
///e.g. backend api versions or special events that are not fully tested, so a
///bug can be easily spotted from the programs log.
template<typename... Args>
inline void log(Args&&... args) { logLogger()->output(std::forward<Args>(args)...); }

///Can be used to output additional information for debug builds.
///Is intended as debugging mechanism for developers and should not be used to log
///information that could be useful to spot a bug from a log provided by a user.
///Note that if the application is not compiled in debug mode (i.e. NDEBUG is set), this
///will no effect or runtime cost at all.
template<typename... Args>
inline void debug(Args&&... args)
{
#ifdef NDEBUG
	nytl::unused(args...); //non-debug build
#else
	debugLogger()->output(std::forward<Args>(args)...); //debug build
#endif
}

}
