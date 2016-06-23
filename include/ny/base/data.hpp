#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>

#include <nytl/any.hpp>
#include <nytl/compFunc.hpp>
#include <nytl/callback.hpp>

#include <vector>
#include <memory>

namespace ny
{

namespace eventType
{
    constexpr unsigned int dataOffer = 25;
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

///Class that allows app to retrieve data from other apps
///The DataOffer interface is usually implemented by the backends and will be passed to the
///application either as result from a clipboard request or with a DataOfferEvent, if there
///was data dropped onto a window.
///It can then be used to determine the different data types in which the data can be represented
///or to retrieve the data in a supported format.
class DataOffer
{
public:
	using DataFunc = CompFunc<void(DataOffer& off, std::uint8_t fmt, const std::any& data)>;
	Callback<bool(DataOffer& off, std::uint8_t fmt)> onFormat;

public:
	virtual ~DataOffer() = default;

	///Returns all currently known supported data representation formats.
	///Note that on some backends the supported types are queried async, therefore
	///this list might grow over time, add a function to the onFormat callback to
	///retrieve new supported types.
	virtual DataTypes types() const = 0;

	///Requests conversion of the data into the given format and registers a function
	///that should be asynchronous called when the data arrives.
	///On some backends, the given function will called immedietly, returning
	///a disconnected connection while on other backends a valid one is returned and the
	///function might be called at some point in the future from the backend thread.
	///The Connection will be automatically destroyed after the function has been
	///called once or earlier.
	///If the requested format cannot be retrieved, the function will be called with an
	///empty any object.
	virtual Connection data(std::uint8_t fmt, const DataFunc& func) = 0;
};

///Event which will be sent when the application recieves data from another application.
///If the event is sent as effect from a drag and drop action, the event will be sent
///to the window on which the data was dropped, otherwise (e.g. clipboard) it will
///be sent to the specified event handler.
class DataOfferEvent : public EventBase<eventType::dataOffer, DataOfferEvent>
{
public:
    DataOfferEvent(EventHandler* handler = {}, std::unique_ptr<DataOffer> xoffer = {})
		: EvBase(handler), offer(std::move(xoffer)) {}
	~DataOfferEvent() = default;

	DataOfferEvent(DataOfferEvent&&) noexcept = default;
	DataOfferEvent& operator=(DataOfferEvent&&) noexcept = default;

    std::unique_ptr<DataOffer> offer;
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
