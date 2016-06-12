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

///This namespace holds constants for all data formats in which data from a DataSource/DataOffer
///may be represented.
namespace dataType
{
	constexpr std::uint8_t raw = 2; //DataObject, raw unspecified data buffer
	constexpr std::uint8_t text = 3; //std::string encoded utf8
    constexpr std::uint8_t filePaths = 4; //std::vector<c++17 ? std::path : std::string>
	constexpr std::uint8_t image = 5; //ny:Image
	constexpr std::uint8_t time = 6; //ny(tl)::Timepoint

	constexpr std::uint8_t html = 11; //std::string encoded utf8 with html
	constexpr std::uint8_t xml = 12; //std::string encoded utf8 with xml
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
    DataOfferEvent(std::unique_ptr<DataOffer> offer) : data(std::move(offer)) {}
	~DataOfferEvent() = default;

    std::unique_ptr<DataOffer> data;
	//XXX:some source indication? clipboard or dnd?
};

///Converts the given string to a dataType constant. If the given string is not recognized,
///0 is returned.
std::uint8_t stringToDataType(const std::string& type);

///Returns a number of string that describe the given data type.
///Will return an empty vector if the type is not known.
std::vector<std::string> dataTypeToString(std::uint8_t type);

///Gives a number of strings for a given DataTypes object.
///Will return an empty vector if the DataType holds no known types.
std::vector<std::string> dataTypesToString(DataTypes types);

}
