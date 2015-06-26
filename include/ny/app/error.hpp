#pragma once

#include <ny/include.hpp>

#include <string>

namespace ny
{

class error : std::exception
{
public:
    enum errorType
    {
        Warning = (1 << 3),
        Critical = (1 << 4),
        Std = (1 << 6),
    };

    error(errorType xtype, std::string xmsg);
    error(std::exception ex);

    errorType type;

    std::string msg;

    const char * what () const throw ();
};

std::string errTypeToString(error::errorType);
void defaultErrorHandler(const error&);

void sendError(error err);
void sendWarning(std::string msg);
void sendCritical(std::string msg);
void sendException(std::exception ex);

extern void(*errorHandler)(const error&);

}
