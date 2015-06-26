#include <ny/app/error.hpp>
#include <ny/app/app.hpp>

#include <iostream>

namespace ny
{
    void(*errorHandler)(const error&) = &defaultErrorHandler;

    void defaultErrorHandler(const error& err)
    {
        std::cout << errTypeToString(err.type) << " Error: " << err.what() << std::endl;

        if(err.type == error::Critical)
        {
            if(!getMainApp())
                return getMainApp()->errorRun(err);

            else
            {
                exit(-1);
            }
        }
    }

    //send
    void sendError(error err)
    {
        errorHandler(err);
    }

    void endStdError(std::exception ex)
    {
        sendError(error(ex));
    }

    void sendCritical(std::string msg)
    {
        sendError(error(error::Critical, msg));
    }

    void sendWarning(std::string msg)
    {
        sendError(error(error::Warning, msg));
    }

    //error
    error::error(errorType xtype, std::string xmsg) : type(xtype), msg(xmsg)
    {
    }

    error::error(std::exception ex) : type(Std), msg(ex.what())
    {
    }

    const char * error::what () const throw ()
    {
        return msg.c_str();
    }


    //errTypeToString
    std::string errTypeToString(error::errorType e)
    {
        if(e == error::Warning)
        {
            return "Warning";
        }
        else if(e == error::Critical)
        {
            return "Critical";
        }
        else if(e == error::Std)
        {
            return "Std";
        }

        return "";
    }
}
