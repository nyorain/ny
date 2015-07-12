#include <ny/keyboard.hpp>

namespace ny
{

std::bitset<255> keyboard::states;
bool keyboard::capsLockState = 0;

char keyboard::toChar(keyboard::key k)
{
    if(shiftActive())
    {
        switch(k)
        {
            case(key::num0): return '0';
            case(key::num1): return '1';
            case(key::num2): return '2';
            case(key::num3): return '3';
            case(key::num4): return '4';
            case(key::num5): return '5';
            case(key::num6): return '6';
            case(key::num7): return '7';
            case(key::num8): return '8';
            case(key::num9): return '9';
            case(key::a): return 'A';
            case(key::b): return 'B';
            case(key::c): return 'C';
            case(key::d): return 'D';
            case(key::e): return 'E';
            case(key::f): return 'F';
            case(key::g): return 'G';
            case(key::h): return 'H';
            case(key::i): return 'I';
            case(key::j): return 'J';
            case(key::k): return 'K';
            case(key::l): return 'L';
            case(key::m): return 'M';
            case(key::n): return 'N';
            case(key::o): return 'O';
            case(key::p): return 'P';
            case(key::q): return 'Q';
            case(key::r): return 'R';
            case(key::s): return 'S';
            case(key::t): return 'T';
            case(key::u): return 'U';
            case(key::v): return 'V';
            case(key::w): return 'W';
            case(key::x): return 'X';
            case(key::y): return 'Y';
            case(key::z): return 'Z';
            case(key::dot): return '.';
            case(key::comma): return ',';
            case(key::space): return ' ';
            default: return (char) 0;
        }
    }

    else
    {
        switch(k)
        {
            case(key::num0): return '0';
            case(key::num1): return '1';
            case(key::num2): return '2';
            case(key::num3): return '3';
            case(key::num4): return '4';
            case(key::num5): return '5';
            case(key::num6): return '6';
            case(key::num7): return '7';
            case(key::num8): return '8';
            case(key::num9): return '9';
            case(key::a): return 'a';
            case(key::b): return 'b';
            case(key::c): return 'c';
            case(key::d): return 'd';
            case(key::e): return 'e';
            case(key::f): return 'f';
            case(key::g): return 'g';
            case(key::h): return 'h';
            case(key::i): return 'i';
            case(key::j): return 'j';
            case(key::k): return 'k';
            case(key::l): return 'l';
            case(key::m): return 'm';
            case(key::n): return 'n';
            case(key::o): return 'o';
            case(key::p): return 'p';
            case(key::q): return 'q';
            case(key::r): return 'r';
            case(key::s): return 's';
            case(key::t): return 't';
            case(key::u): return 'u';
            case(key::v): return 'v';
            case(key::w): return 'w';
            case(key::x): return 'x';
            case(key::y): return 'y';
            case(key::z): return 'z';
            case(key::dot): return '.';
            case(key::comma): return ',';
            case(key::space): return ' ';
            default: return (char) 0;
        }
    }

}

bool keyboard::isKeyPressed(keyboard::key id)
{
    if(id == keyboard::key::none) return 0;

    return states[(unsigned int)id];
}

void keyboard::pressKey(keyboard::key id)
{
    if(id == keyboard::key::none) return;

    if(id == keyboard::key::capsLock)
    {
        capsLockState = !capsLockState;
    }

    states[(unsigned int)id] = 1;
}

void keyboard::releaseKey(keyboard::key id)
{
    if(id == keyboard::key::none) return;

    states[(unsigned int)id] = 0;
}

bool keyboard::shiftActive()
{
    if(isKeyPressed(key::leftshift) + isKeyPressed(key::rightshift) + capsLockState == 1 || isKeyPressed(key::leftshift) + isKeyPressed(key::rightshift) + capsLockState == 3)
        return 1;

    return 0;
}

bool keyboard::altActive()
{
    if(isKeyPressed(key::leftalt) || isKeyPressed(key::rightalt))
        return 1;

    return 0;
}

bool keyboard::ctrlActive()
{
    if(isKeyPressed(key::leftctrl) || isKeyPressed(key::rightctrl))
        return 1;

    return 0;
}

}
