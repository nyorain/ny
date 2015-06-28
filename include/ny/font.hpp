#pragma once

#include <string>

namespace ny
{

//todo: 1 standrard default font

class freeTypeFont;
class cairoFont;
class gdiFont;

class font
{
protected:
    bool fromFile_;
    std::string name_; //can conatain file name or fontname

    freeTypeFont* ftFont_ = nullptr;
    cairoFont* cairoFont_ = nullptr;

    #ifdef WithWinapi
    gdiFont* gdiFont_ = 0;
    #endif //Winapi

public:
    ~font();

    void loadFromFile(const std::string& filename);
    void loadFromName(const std::string& fontname);

    //the bool parameter specifies if the font handle should be created if it does not exist
    freeTypeFont* getFreeTypeHandle(bool cr = 1);
    cairoFont* getCairoHandle(bool cr = 1);

    #ifdef WithWinapi
    gdiFont* getGDIHandle(bool cr = 1);
    #endif //Winapi
public:
    static font defaultFont;
    static font* getDefault(){ return &defaultFont; };
};

}
