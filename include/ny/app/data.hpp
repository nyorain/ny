#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>

#include <nytl/any.hpp>

#include <vector>
#include <memory>

namespace ny
{

namespace eventType
{
    constexpr unsigned int dataReceive = 25;
}

///This namespace holds constants for all datatypes.
///Since a namespace with constexpr declarations was chosen instead of an enum, applications
///can extend this collection with their own dataTypes.
///The given data types do mainly represent the most familiar mime types used for data transfer.
namespace dataType
{
    constexpr std::uint8_t allImage = 1; //can provide all image formats
    constexpr std::uint8_t allAudio = 2; //can provide all audio formats
    constexpr std::uint8_t allVideo = 3; //can provide all video formats
	constexpr std::uint8_t allText = 4; //can provide all text formats

    namespace text
    {
        constexpr std::uint8_t plain = 6; //std::string
        constexpr std::uint8_t utf8 = 7; //std::string
        constexpr std::uint8_t utf16 = 8; //std::u16string
        constexpr std::uint8_t utf32 = 9; //std::u32string
    }

    namespace image
    {
        constexpr std::uint8_t png = 10; //ny::Image
        constexpr std::uint8_t jpeg = 11; //ny::Image
        constexpr std::uint8_t gif = 12; //ny::AnimatedImage
        constexpr std::uint8_t tiff = 13; //ny::Image
        constexpr std::uint8_t bmp = 14; //ny::Image
        constexpr std::uint8_t svg = 15; //ny::SvgImage
    }

    namespace audio //usually a buffer, in future maybe some 3rd lib audio object
    {
        constexpr std::uint8_t mpeg = 21;
        constexpr std::uint8_t aac = 22;
        constexpr std::uint8_t flac = 23;
        constexpr std::uint8_t webm = 24;
        constexpr std::uint8_t mp4 = 25;
        constexpr std::uint8_t ogg = 26;
        constexpr std::uint8_t wave = 26;
    }

    namespace video //usually a buffer, in future maybe some 3rd lib video object
    {
        constexpr std::uint8_t mp4 = 31;
        constexpr std::uint8_t mpeg = 32;
        constexpr std::uint8_t avi = 33;
        constexpr std::uint8_t ogg = 34;
        constexpr std::uint8_t webm = 35;
        constexpr std::uint8_t quicktime = 36;
        constexpr std::uint8_t flv = 37;
    }

    namespace app //usually a buffer (DataObject) holding the data of the file
    {
        constexpr std::uint8_t atom = 41;
        constexpr std::uint8_t ogg = 42;
        constexpr std::uint8_t pdf = 43;
        constexpr std::uint8_t xml = 44;
        constexpr std::uint8_t zip = 45;
        constexpr std::uint8_t font = 46;
        constexpr std::uint8_t json = 47;
    }

	constexpr std::uint8_t raw = 51; //DataObject
    constexpr std::uint8_t filePath = 4; //c++17 ? std::path : std::string
}


///Represents multiple data type formats in which certain data can be retrieved.
///Used by DataSource and DataOffer to signal in which types the data is available.
///The constant types used should be defined as constexpr std::uint8_t in the ny::dataType
///namespace. ny already provides the most common datatypes, applications can extent them with
///their own definitions starting with number 100.
///\sa dataTypes
class DataTypes
{
public:
    std::vector<std::uint8_t> types;

public:
    void add(std::uint8_t type);
    void remove(std::uint8_t type);
    bool contains(std::uint8_t type) const;
};

///Struct used to represent owned raw data.
struct DataObject
{
    std::unique_ptr<std::uint8_t> data;
    std::size_t size = 0;
};

///The DataSource class is an interface implemented by the application to start drag and drop
///actions or copy data into the clipboard.
///The interface gives information about in which formats data can be represented and then
///provides the data for a given format.
class DataSource
{
public:
    virtual ~DataSource() = default;

	///Returns all supported dataTypes format constants as a DataTypes object.
	virtual DataTypes types() const = 0;

	///Returns a std::any holding the data in the given format.
	///The std::any must contain a object of the type specified at the dataType constant
	///declaration.
	virtual std::any data(unsigned int format) const = 0;
};

//XXX: useful? should exist?
//http://stackoverflow.com/questions/3261379/getting-html-source-or-rich-text-from-the-x-clipboard
///Default DataSource implementation.
class DefaultDataSource : public DataSource
{
public:
	template<typename T>
	DefaultDataSource(const T& data, DataTypes types) : types_(types), data_(data)  {}
	~DefaultDataSource() = default;

	virtual DataTypes types() const override;
	virtual std::any data(unsigned int format) const override;

protected:
	DataTypes types_;
	std::any data_;
};

///Class that allows app to retrieve data from other apps
///The DataOffer interface is usually implemented by the backends and will be passed to the
///application either as result from a clipboard request or with a DataOfferEvent, if there
///was data dropped onto a window.
///It can then be used to determine the different data types in which the data can be represented
///or to retrieve the data in a supported format.
class DataOffer
{
public:
	virtual ~DataOffer() = default;

	virtual DataTypes types() const = 0;
	virtual std::any data(std::uint8_t format) const = 0;
};

///Event which will be sent when the application recieves data from another application.
///If the event is sent as effect from a drag and drop action, the event will be sent
///to the window on which the data was dropped, otherwise (e.g. clipboard) it will
///be sent to the specified event handler.
class DataOfferEvent : public EventBase<eventType::dataReceive, DataReceiveEvent>
{
public:
    DataReceiveEvent(std::unique_ptr<DataOffer> offer) : data(std::move(offer)) {}
	~DataOfferEvent() = default;

    std::unique_ptr<DataOffer> data;
	//XXX:some source indication? clipboard or dnd?
};

///Converts the given string to a dataType constant. If the given string is not recognized,
///0 is returned.
std::uint8_t stringToDataType(const std::string& type);

///Gives a number of strings for a given dataTypes constant.
///\param onlyMime If set to true, only mime type strings are returned.
std::vector<std::string> dataTypeToString(std::uint8_t type, bool onlyMime = 0);

///Gives a number of strings for a given DataTypes object.
///\param onlyMime If set to true, only mime type strings are returned.
std::vector<std::string> dataTypesToString(DataTypes types, bool onlyMime = 0);

}
