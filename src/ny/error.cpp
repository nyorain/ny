#include <ny/error.hpp>
#include <ny/app.hpp>

#include <iostream>

namespace ny
{

std::ostream& warningStream = std::cout;
std::ostream& debugStream = std::cout;
std::ostream& errorStream = std::cerr;

void nyError()
{
    if(nyMainApp())
    {
        nyMainApp()->onError();
        return;
    }

    std::cerr << "Error ocurred. Continue? (y/n)" << std::endl;
    std::string s;
    std::cin >> s;

    if(s == "y" || s == "1" || s == "yes")
    {
        return;
    }

    if(!(s == "n" || s == "0" || s == "no"))
        std::cerr << "invalid option. Exiting" << std::endl;

    exit(0);
}

}
