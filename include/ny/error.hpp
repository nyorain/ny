#pragma once

#include <ny/include.hpp>
#include <ny/util/misc.hpp>

#include <string>
#include <stdexcept>
#include <ostream>

namespace ny
{

//todo÷ make pointer
extern std::ostream& warningStream;
extern std::ostream& debugStream;
extern std::ostream& errorStream;

template<class ... Args> void sendWarning(Args ... info){ printVar(warningStream, info ...); warningStream << std::endl; }
template<class ... Args> void sendDebug(Args ... info){
    #ifdef NY_DEBUG
        printVar(debugStream, info ...); debugStream << std::endl;
    #endif // NY_DEBUG
}

void sendError();

inline void sendError(const std::exception& err){ errorStream << err.what() << std::endl; sendError(); }
template<class ... Args> void sendError(Args ... info){ printVar(errorStream, info ...); errorStream << std::endl; sendError(); }
template<class ... Args> void sendError(Args ... info, const std::exception& err){ printVar(errorStream, info ...); errorStream << "\n" << err.what() << std::endl; sendError(); }

}
