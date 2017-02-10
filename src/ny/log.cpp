// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/log.hpp>
#include <nytl/utf.hpp>

#include <iostream>

#ifdef NY_WithWinapi
	#include <windows.h>
#endif //WithWinapi

namespace ny {
namespace {
	// const auto defaultOutBuf = std::cout.rdbuf();
	// const auto defaultErrBuf = std::cerr.rdbuf();
	// const auto defaultLogBuf = std::clog.rdbuf();
} // anonymous util namespace

void writeStream(std::ostream& os, const std::string& text)
{
	#ifdef NY_WithWinapi
		HANDLE handle;

		if(os.rdbuf() == defaultOutBuf)
			handle = ::GetStdHandle(STD_OUTPUT_HANDLE);

		if(os.rdbuf() == defaultErrBuf || os.rdbuf() == defaultLogBuf)
			handle = ::GetStdHandle(STD_ERROR_HANDLE);

		if(handle)
		{
			auto str2 = nytl::toUtf16(text);
			::WriteConsoleW(handle, str2.c_str(), str2.size(), nullptr, nullptr);
			return;
		}
	#endif

	os << text;
}

std::unique_ptr<LoggerBase>& warningLogger()
{
	static std::unique_ptr<LoggerBase> object =
		std::make_unique<Logger<std::ostream>>(std::cout, "ny::warning: ");

	return object;
}

std::unique_ptr<LoggerBase>& logLogger()
{
	static std::unique_ptr<LoggerBase> object =
		std::make_unique<Logger<std::ostream>>(std::clog, "ny::log: ");

	return object;
}

std::unique_ptr<LoggerBase>& errorLogger()
{
	static std::unique_ptr<LoggerBase> object =
		std::make_unique<Logger<std::ostream>>(std::cerr, "ny::error: ");

	return object;
}

std::unique_ptr<LoggerBase>& debugLogger()
{
#ifdef NDEBUG
	static std::unique_ptr<LoggerBase> object =
		std::make_unique<DummyStream>();

	return object;

#else
	static std::unique_ptr<LoggerBase> object =
		std::make_unique<Logger<std::ostream>>(std::cout, "ny::debug: ");

	return object;
#endif
}

} // namespace ny
