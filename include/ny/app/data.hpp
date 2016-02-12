#pragma once

#include <ny/include.hpp>
#include <ny/app/event.hpp>

#include <iostream>
#include <vector>
#include <functional>
#include <fstream>

namespace ny
{

namespace eventType
{
    constexpr unsigned int dataReceive = 25;
}

namespace dataType
{
    const unsigned char allImage = 1;
    const unsigned char allAudio = 2;
    const unsigned char allVideo = 3;
    const unsigned char filePath = 4;

    namespace text
    {
        const unsigned char plain = 6;
        const unsigned char utf8 = 7;
        const unsigned char utf16 = 8;
        const unsigned char utf32 = 9;
    }

    namespace image
    {
        const unsigned char png = 10;
        const unsigned char jpeg = 11;
        const unsigned char gif = 12;
        const unsigned char tiff = 13;
        const unsigned char bmp = 14;
        const unsigned char svg = 15;
    }

    namespace audio
    {
        const unsigned char mpeg = 21;
        const unsigned char aac = 22;
        const unsigned char flac = 23;
        const unsigned char webm = 24;
        const unsigned char mp4 = 25;
        const unsigned char ogg = 26;
        const unsigned char wave = 26;
    }

    namespace video
    {
        const unsigned char mp4 = 31;
        const unsigned char mpeg = 32;
        const unsigned char avi = 33;
        const unsigned char ogg = 34;
        const unsigned char webm = 35;
        const unsigned char quicktime = 36;
        const unsigned char flv = 37;
    }

    namespace app
    {
        const unsigned char atom = 41;
        const unsigned char ogg = 42;
        const unsigned char pdf = 43;
        const unsigned char xml = 44;
        const unsigned char zip = 45;
        const unsigned char font = 46;
        const unsigned char json = 47;
    }
}


//dataTypes
class DataTypes
{
protected:
    std::vector<unsigned char> types_;

public:
    void addType(unsigned char type);
    void removeType(unsigned char type);

    bool contains(unsigned char type) const;

    std::vector<unsigned char> getvector() const { return types_; }
};

//dataObject
//used by the  to send other applications data
class DataObject
{
public:
    void* data;
    unsigned int size; //size in bytes
};

class DataSource
{
protected:
	//std::function<dataObject(unsigned char)> converter_;
    //dataTypes types_;

    //dataSource() : converter_([](unsigned char){ return dataObject {nullptr, 0}; }) {};

public:
    //dataSource(std::function<dataObject(unsigned char)> convertCB, const dataTypes& types = dataTypes()) : converter_(convertCB), types_(types) {}
    //virtual ~dataSource(){}

    //virtual dataTypes getPossibleTypes(){ return types_; };

	//virtual dataObject getData(unsigned char type){ return converter_(type); };
};

//dataOffer
//abstract class defined by the
//makes it possible to recieve data from other applications
class DataOffer
{
public:
	virtual ~DataOffer() = default;

	//virtual bool getPossibleTypes(std::function<void(const dataTypes&)> func) = 0;

	//virtual bool getData(unsigned char format, std::function<void(const std::string&)> func) = 0;
	//virtual bool getData(unsigned char format, std::function<void(const Image&)> func) = 0;
	//virtual bool getData(unsigned char format, std::function<void(const File&)> func) = 0;
	//virtual bool getData(unsigned char format, std::function<void(const DataObject&)> func) = 0;
};

//event
class DataReceiveEvent : public EventBase<eventType::dataReceive, DataReceiveEvent>
{
public:
    //dataReceiveEvent(std::unique_ptr<dataOffer> d) : evBase(), data(std::move(d)) {}
    //~dataReceiveEvent() = default;

    //std::unique_ptr<dataOffer> data;
};

unsigned char stringToDataType(const std::string& type);
std::vector<std::string> dataTypeToString(unsigned char type, bool onlyMime = 0);
std::vector<std::string> dataTypesToString(DataTypes types, bool onlyMime = 0);

}

