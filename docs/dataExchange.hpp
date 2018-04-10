// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/image.hpp> // ny::Image

#include <nytl/callback.hpp> // nytl::Callback
#include <nytl/span.hpp> // nytl::Span

#include <vector> // std::vector
#include <memory> // std::unique_ptr
#include <functional> // std::function
#include <variant> // std::variant

namespace ny {
namespace mime {

constexpr auto text = "text/plain"; // text with unknown decoding
constexpr auto utf8Text = "text/plain;charset=utf-8"; // utf-8 text
constexpr auto uriList = "text/uri-list"; // uri/file list
constexpr auto image = "image/x-ny-data"; // raw ny image
constexpr auto raw = "application/octet-stream"; // binary data

} // namespace mime

/// Possible data transfer types.
using ExchangeData = std::variant<
	std::monostate, // invalid, empty
	std::string, // all "text/*" types except "text/uri-list"
	std::vector<std::string>, // "text/uri-list"
	UniqueImage, // "image/x-ny-data"
	std::vector<std::byte>>; // everything else

/// Can be implemented by an application to provide data to another application.
class DataSource {
public:
	virtual ~DataSource() = default;

	/// Returns all mime types in which it can be represented.
	virtual std::vector<std::string> formats() const = 0;

	/// Returns data in the given format.
	/// Should return a monostate (i.e. empty) variant on error.
	/// Note that the valid member in the returned ExchangeData must otherwise
	/// met the requested format (see ExchangeData typedef), i.e.
	/// return a variant holding a std::string for format "image/jpg" is
	/// invalid.
	virtual ExchangeData data(const char* format) const = 0;

	/// Returns an image representing the data. This image could e.g. used
	/// when this DataSource is used for a drag and drop opertation.
	/// If the data cannot be represented using an image, return a 
	/// default-constructed Image object (or just don't override it).
	virtual Image image() const { return {}; }
};

/// Class that allows app to retrieve data from other apps
/// The DataOffer interface is usually implemented by the backends and will 
/// be passed to the application either as result from a clipboard request 
/// or when data was dropped onto a window.
/// It can then be used to determine the different data types in which the 
/// data can be represented or to retrieve the data in a supported format.
/// On Destruction, the DataOffer should trigger all waiting data callback 
/// without data to end the waiting.
class DataOffer {
public:
	using FormatsListener = std::function<void(nytl::Span<const char*>)>;
	using DataListener = std::function<void(ExchangeData)>;

public:
	DataOffer() = default;

	/// When a DataOffer is destructed it should call all registered Listeners
	/// with an empty object (i.e. signaling failure).
	virtual ~DataOffer() = default;

	/// Requests all supported formats in which the data is offered.
	/// Will return on error, otherwise the FormatsListener will be
	/// called either from within this function or later on.
	virtual bool formats(FormatsListener&) = 0;

	/// Requests the offered data in the given mime type.
	/// If this failed because the format is not supported or another error
	/// ocurred, false should be returned. Otherwise the given DataListener
	/// should be called later on from within a AppContext dispatch function
	/// or instantly before this call returns.
	/// Note that DataOffers should not cache data internally, this function
	/// is not expected to be called multiple times for one format.
	virtual bool data(const char* format, DataListener&) = 0;
};

// - The following functions are mainly used by backends -
/// Serializes the given image (size, stride, (int) format, data).
std::vector<uint8_t> serialize(const Image&);

/// Tries to interpret the given data buffer as serialized image.
/// Throws an exception on error.
UniqueImage deserializeImage(nytl::Span<const uint8_t> buffer);

/// Encodes a vector of uris to a single string with mime-type text/uri-list 
/// encoded in utf8. Will replace special chars with their escape codes and 
/// seperate the given uris using newlines.
std::string encodeUriList(const std::vector<std::string>& uris);

/// Decodes a given utf8 encoded string of mime-type text/uri-list to a vector
/// holding the uri list items.
/// Will replace '%' escape codes in the list with utf8 special chars.
/// \param removeComments removes (comment) uri lines that start with a '#'
std::vector<std::string> decodeUriList(const std::string& list, 
	bool removeComments = true);

/// Returns an ExchangeData object that holds the correctly formatted and
/// wrapped value for the given format and raw data buffer.
ExchangeData wrap(std::vector<uint8_t> rawBuffer, const char* format);

/// Returns a raw buffer for the given ExchangeData.
std::vector<std::byte> unwrap(const ExchangeData& data);

} // namespace ny
