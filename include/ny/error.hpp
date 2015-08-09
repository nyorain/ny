#pragma once

#include <ny/include.hpp>
#include <nyutil/misc.hpp>

#include <string>
#include <stdexcept>
#include <ostream>

namespace ny
{

//todo÷ make pointer
extern std::ostream& warningStream;
extern std::ostream& debugStream;
extern std::ostream& errorStream;

template<class ... Args> void sendWarning(Args ... info){ printVars(warningStream, info ...); warningStream << std::endl; }
template<class ... Args> void sendDebug(Args ... info){
    #ifdef NY_DEBUG
        printVars(debugStream, info ...); debugStream << std::endl;
    #endif // NY_DEBUG
}

void sendError();

inline void sendError(const std::exception& err){ errorStream << err.what() << std::endl; sendError(); }
template<class ... Args> void sendError(Args ... info){ printVars(errorStream, info ...); errorStream << std::endl; sendError(); }
template<class ... Args> void sendError(Args ... info, const std::exception& err){ printVars(errorStream, info ...); errorStream << "\n" << err.what() << std::endl; sendError(); }

}
