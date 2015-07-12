#pragma once

#include <ny/include.hpp>
#include <string>

namespace ny
{

//todo: 1 standrard default font
class font
{
protected:
    bool fromFile_;
    std::string name_; //can conatain file name or fontname


    #ifdef NY_WithFreeType
    freeTypeFont* ftFont_ = nullptr;
    #endif //NY_WithFreeType


    #ifdef NY_WithCairo
    cairoFont* cairoFont_ = nullptr;
    #endif //NY_WithCairo


    #ifdef NY_WithWinapi
    gdiFont* gdiFont_ = 0;
    #endif //NY_Winapi

public:
    ~font();

    void loadFromFile(const std::string& filename);
    void loadFromName(const std::string& fontname);


    //the bool parameter specifies if the font handle should be created if it does not exist
    #ifdef NY_WithFreeType
    freeTypeFont* getFreeTypeHandle(bool cr = 1);
    #endif //NY_Winapi


    #ifdef NY_WithCairo
    cairoFont* getCairoHandle(bool cr = 1);
    #endif //NY_Winapi


    #ifdef NY_WithWinapi
    gdiFont* getGDIHandle(bool cr = 1);
    #endif //NY_Winapi

public:
    static font defaultFont;
    static font* getDefault(){ return &defaultFont; };
};

}
