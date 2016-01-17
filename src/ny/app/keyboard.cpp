#include <ny/app/keyboard.hpp>

namespace ny
{

std::bitset<255> Keyboard::states_;
std::string Keyboard::utf8(Keyboard::Key k)
{
    if(capsActive())
    {
        switch(k)
        {
            case(Key::num0): return "0";
            case(Key::num1): return "1";
            case(Key::num2): return "2";
            case(Key::num3): return "3";
            case(Key::num4): return "4";
            case(Key::num5): return "5";
            case(Key::num6): return "6";
            case(Key::num7): return "7";
            case(Key::num8): return "8";
            case(Key::num9): return "9";
            case(Key::a): return "A";
            case(Key::b): return "B";
            case(Key::c): return "C";
            case(Key::d): return "D";
            case(Key::e): return "E";
            case(Key::f): return "F";
            case(Key::g): return "G";
            case(Key::h): return "H";
            case(Key::i): return "I";
            case(Key::j): return "J";
            case(Key::k): return "K";
            case(Key::l): return "L";
            case(Key::m): return "M";
            case(Key::n): return "N";
            case(Key::o): return "O";
            case(Key::p): return "P";
            case(Key::q): return "Q";
            case(Key::r): return "R";
            case(Key::s): return "S";
            case(Key::t): return "T";
            case(Key::u): return "U";
            case(Key::v): return "V";
            case(Key::w): return "W";
            case(Key::x): return "X";
            case(Key::y): return "Y";
            case(Key::z): return "Z";
            case(Key::dot): return ".";
            case(Key::comma): return ",";
            case(Key::space): return " ";
            default: return "";
        }
    }

    else
    {
        switch(k)
        {
            case(Key::num0): return "0";
            case(Key::num1): return "1";
            case(Key::num2): return "2";
            case(Key::num3): return "3";
            case(Key::num4): return "4";
            case(Key::num5): return "5";
            case(Key::num6): return "6";
            case(Key::num7): return "7";
            case(Key::num8): return "8";
            case(Key::num9): return "9";
            case(Key::a): return "a";
            case(Key::b): return "b";
            case(Key::c): return "c";
            case(Key::d): return "d";
            case(Key::e): return "e";
            case(Key::f): return "f";
            case(Key::g): return "g";
            case(Key::h): return "h";
            case(Key::i): return "i";
            case(Key::j): return "j";
            case(Key::k): return "k";
            case(Key::l): return "l";
            case(Key::m): return "m";
            case(Key::n): return "n";
            case(Key::o): return "o";
            case(Key::p): return "p";
            case(Key::q): return "q";
            case(Key::r): return "r";
            case(Key::s): return "s";
            case(Key::t): return "t";
            case(Key::u): return "u";
            case(Key::v): return "v";
            case(Key::w): return "w";
            case(Key::x): return "x";
            case(Key::y): return "y";
            case(Key::z): return "z";
            case(Key::dot): return ".";
            case(Key::comma): return ",";
            case(Key::space): return " ";
            default: return "";
        }
    }

}

bool Keyboard::keyPressed(Keyboard::Key id)
{
    if(id == Keyboard::Key::none) return 0;

    return states_[static_cast<unsigned int>(id)];
}

void Keyboard::keyPressed(Keyboard::Key id, bool pressed)
{
    if(id == Keyboard::Key::none) return;

    states_[static_cast<unsigned int>(id)] = pressed;
}

/*
bool Keyboard::capsActive()
{
    if(isKeyPressed(key::leftshift) + isKeyPressed(key::rightshift) + capsLockState == 1 || isKeyPressed(key::leftshift) + isKeyPressed(key::rightshift) + capsLockState == 3)
        return 1;

    return 0;
}

bool Keyboard::altActive()
{
    if(isKeyPressed(key::leftalt) || isKeyPressed(key::rightalt))
        return 1;

    return 0;
}

bool Keyboard::ctrlActive()
{
    if(isKeyPressed(key::leftctrl) || isKeyPressed(key::rightctrl))
        return 1;

    return 0;
}
*/

}
