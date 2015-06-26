#pragma once

#include "include.hpp"
#include "app/event.hpp"
#include <iostream>
#include <vector>
#include <functional>

namespace ny
{

namespace eventType
{
    const unsigned char dataReceive = 25;
}

namespace dataType
{
    const unsigned char allImage = 1;
    const unsigned char text = 2;
    const unsigned char filePath = 3;

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

    namespace application
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
class dataTypes
{
protected:
    std::vector<unsigned char> types_;

public:
    void addType(unsigned char type);
    void removeType(unsigned char type);

    bool contains(unsigned char type) const;

    std::vector<unsigned char> getVector() const { return types_; }
};

//dataObject
//used by the backends to send other applications data
class dataSource
{
protected:
	dataTypes types_;

public:
    virtual ~dataSource(){}

    virtual dataTypes getPossibleTypes() = 0;

    virtual bool getAsString(unsigned char type, std::function<void(const std::string&)> func) = 0;
    virtual bool getAsImage(unsigned char type, std::function<void(const image&)> func) = 0;
    template<class T> bool get(unsigned char type, std::function<void(const T&)> func){ return getData(type, [func](void* d){ func(*((T*)d)); }); }
};

//dataOffer
//abstract class defined by the backends
//makes it possible to recieve data from other applications
class dataOffer
{
public:
	virtual ~dataOffer(){}

	virtual bool getPossibleTypes(std::function<void(const dataTypes&)> func) = 0;

	virtual bool getData(unsigned char format, std::function<void(const std::string&)> func) = 0;
	virtual bool getData(unsigned char format, std::function<void(const image&)> func) = 0;
	virtual bool getData(unsigned char format, std::function<void(const file&)> func) = 0;
	virtual bool getData(unsigned char format, std::function<void(void*)> func) = 0;
};

//event
class dataReceiveEvent : public event
{
public:
    dataReceiveEvent(dataOffer& d) : event(eventType::dataReceive), data(d) {}
    ~dataReceiveEvent(){ delete (&data); }

    dataOffer& data;
};

unsigned char stringToDataType(const std::string& type);
std::vector<std::string> dataTypeToString(unsigned char type);
std::vector<std::string> dataTypesToString(dataTypes types);

}

