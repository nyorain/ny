// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/event.hpp>
#include <ny/imageData.hpp>
#include <ny/asyncRequest.hpp>

#include <nytl/callback.hpp>
#include <nytl/stringParam.hpp>

#include <vector>
#include <memory>
#include <any>

namespace ny
{

using OwnedImageData = BasicImageData<std::unique_ptr<std::uint8_t[]>>;

namespace eventType
{
	constexpr auto dataOffer = 31u;
}

///Description of a data format by mime and non-mime strings.
///There are a few standard formats in which data is passed around (i.e. wrapped
///into the returnd std::any object form DataSource or DataOffer) using special types.
///Data in custom DataFormats are passed around using a raw buffer.
///The names of the standard formats should not be used for custom formats.
///Formats, their mime-type names and the types they should be stored in:
///
/// | Format 	| Non-Owned type 		| Owned type 			| mime-type name				|
/// |-----------|-----------------------|-----------------------|-------------------------------|
/// | raw		| Range<uint8_t>		| vector<uint8_t>		| "application/octet-stream"	|
/// | text		| Range<char>			| string				| "text/plain"					|
/// | uriList	| Range<const char*> 	| vector<string> 		| "text/uri-list"				|
/// | image		| ImageData				| OwnedImageData		| "image/x-ny-data"				|
/// | <custom>  | Range<uint8_t> 		| vector<uint8_t>		|								|
class DataFormat
{
public:
	///The primary default name of the DataFormat.
	///This will be used to compare multiple DataFormats and must not be empty, otherwise
	///the DataFormat is invalid.
	///This should be a mime-type, but does not have to be a standardized one if there is none.
	std::string name;

	///Additional names that this format might be recognized under. Basically a help for
	///other applications that might know it the same format under a different name.
	///More significant names/descriptions should come first. Can also contains none mime-type
	///names, but should be avoided.
	std::vector<std::string> names;

public:
	static const DataFormat none; //empty object, used for invalid formats
	static const DataFormat raw; //raw, not further specified data buffer
	static const DataFormat text; //textual data
	static const DataFormat uriList; //a list of uri objects
	static const DataFormat imageData; //raw image data
};

bool operator==(const DataFormat& a, const DataFormat& b) { return a.name == b.name; }
bool operator!=(const DataFormat& a, const DataFormat& b) { return !(a == b); }

///DataObject that holds a type-erased data object and a description of the type
///and format this data has.
struct DataObject
{
	///The data wrapped into an any object.
	///The other DataObject members specify its type.
	///See the DataFormat type table for more information.
	///If this is empty the DataObject is invalid.
	std::any data {};

	///The format of the data. This can either be one of the standard DataFormats that
	///have a special type associated with them or a custom format.
	DataFormat format {};

	///Specifies whether the data any contains owned or not-owned data.
	///There are usually two different data types for a DataFormat depending on
	///whether the data is owned or not.
	///This can be used to prevent unneeded copies of huge data.
	bool owned {};

	///Default invalid DataObject.
	///Usually used to signal that retrieving data failed or a request was invalid.
	const static DataObject none;
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
	virtual std::vector<DataFormat> types() const = 0;

	///Returns data in the given format and specifies whehter it is the owned type.
	///The std::any must contain a object of the type specified at the dataType constant
	///declaration.
	virtual DataObject data(const DataFormat& format) const = 0;

	///Returns an image representing the data. This image could e.g. used
	///when this DataSource is used for a drag and drop opertation.
	///If the data cannot be represented using an image, return a default-constructed
	///ImageData object (or just don't override it.)
	virtual ImageData image() const { return {}; };
};

///Class that allows app to retrieve data from other apps
///The DataOffer interface is usually implemented by the backends and will be passed to the
///application either as result from a clipboard request or with a DataOfferEvent, if there
///was data dropped onto a window.
///It can then be used to determine the different data types in which the data can be represented
///or to retrieve the data in a supported format.
///On Destruction, the DataOffer should trigger all waiting data callback without data to
///signal them that they dont have to wait for it any longer since retrieval failed.
class DataOffer
{
public:
	using DataFunction = nytl::CompFunc<void(const std::any&, DataOffer&, unsigned int)>;

	using FormatsRequest = std::unique_ptr<AsyncRequest<std::vector<DataFormat>>>;
	using DataRequest = std::unique_ptr<AsyncRequest<DataObject>>;

public:
	///Will be called everytime a new format is signaled.
	// [[deprecated("Use the new AsyncRequest api")]]
	Callback<bool(DataOffer& offer, const DataFormat& format)> onFormat;

public:
	DataOffer() = default;
	virtual ~DataOffer() = default;

	///Returns all currently known supported data representation formats.
	///Note that on some backends the supported types are queried async, therefore
	///this list might grow over time, add a function to the onFormat callback to
	///retrieve new supported types.
	// [[deprecated("Use the new AsyncRequest api")]]
	virtual std::vector<DataFormat> types() const { return {}; };
	virtual FormatsRequest formats() const { throw 0; };

	///Requests conversion of the data into the given format and registers a function
	///that should be asynchronous called when the data arrives.
	///On some backends, the given function will be called immedietly, returning
	///a disconnected connection while on other backends a valid one is returned and the
	///function might be called at some point in the future from the event (backend) thread.
	///The Connection will be automatically destroyed after the function has been
	///called once or earlier.
	///If the requested format cannot be retrieved, the function will be called with an
	///empty any object.
	///Note that on some backends this function might not return (running an internal event loop)
	///until the data is retrieved.
	// [[deprecated("Use the new AsyncRequest api")]]
	virtual nytl::Connection data(const DataFormat&, const DataFunction&) { return {}; }
	virtual DataRequest data(const DataFormat&) { throw 0; }
};

///Event which will be sent when the application recieves data from another application.
///If the event is sent as effect from a drag and drop action, the event will be sent
///to the window on which the data was dropped.
class DataOfferEvent : public EventBase<eventType::dataOffer, DataOfferEvent>
{
public:
	DataOfferEvent(EventHandler* handler = {}, std::unique_ptr<DataOffer> xoffer = {})
		: EvBase(handler), offer(std::move(xoffer)) {}
	~DataOfferEvent() = default;

	DataOfferEvent(DataOfferEvent&&) noexcept = default;
	DataOfferEvent& operator=(DataOfferEvent&&) noexcept = default;

	//events are usually passed around as const (EventHandler::handleEvent) but the
	//handler receiving this might wanna take ownership of the DataOffer implementation
	//which is not possible with a const event.
	mutable std::unique_ptr<DataOffer> offer;
};

std::vector<uint8_t> serialize(const ImageData&);
OwnedImageData deserializeImageData(nytl::Range<uint8_t> buffer);

///Encodes a vector of uris to a single string with mime-type text/uri-list encoded in utf8.
///Will replace special chars with their escape codes and seperate the given uris using
///newlines.
///\sa decodeUriList
std::string encodeUriList(const std::vector<std::string>& uris);

///Decodes a given utf8 encoded string of mime-type text/uri-list to a vector of uris.
///Will replace '%' escape codes in the list with utf8 special chars and ignore comment lines.
///\param removeComments removes uri lines that start with a '#'
///\sa encodeUriList
std::vector<std::string> decodeUriList(const std::string& list, bool removeComments = true);

///Returns an DataObject that wraps the data of a raw buffer in the correct format
///for the given parameters. Does basically check for standard formats and wrap the
///raw buffer otherwise.
DataObject wrap(nytl::Range<uint8_t> rawBuffer, const DataFormat& format, bool owned);

///Returns a raw buffer for the given DataObject and the DataFormat for the data the any wraps.
nytl::Range<uint8_t> unwrap(const DataObject& dataObject, std::vector<uint8_t>& owned);
std::vector<uint8_t> unwrapOwned(const DataObject& dataObject);

// TODO: for additional parameter (e.g. charset) parsing?
// std::any wrap(nytl::Range<uint8_t> rawBuffer, nytl::StringParam formatName);
// std::vector<uint8_t> unwrap(const std::any& any, nytl::StringParam formatName);

}
