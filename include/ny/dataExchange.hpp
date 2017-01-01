// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/image.hpp> // ny::Image

#include <nytl/callback.hpp> // nytl::Callback
#include <nytl/stringParam.hpp> // nytl::StringPram
#include <nytl/span.hpp> // nytl::Span

#include <vector> // std::vector
#include <memory> // std::unique_ptr
#include <functional> // std::function
#include <any> // std::any

namespace ny {

/// Description of a data format by mime and non-mime strings.
/// There are a few standard formats in which data is passed around (i.e. wrapped
/// into the returnd std::any object form DataSource or DataOffer) using special types.
/// Data in custom DataFormats are passed around using a raw buffer.
/// The names of the standard formats should not be used for custom formats.
/// Formats, their mime-type names and the type they should be stored in:
///
/// | Format 	| std::any wrapped type | mime-type name				|
/// |-----------|-----------------------|-------------------------------|
/// | raw		| vector<uint8_t>		| "application/octet-stream"	|
/// | text		| string				| "text/plain"					|
/// | uriList	| vector<string> 		| "text/uri-list"				|
/// | image		| UniqueImage			| "image/x-ny-data"				|
/// | <custom>  | vector<uint8_t>		| <custom>						|
class DataFormat {
public:
	static const DataFormat none; //empty object, used for invalid formats
	static const DataFormat raw; //raw, not further specified data buffer
	static const DataFormat text; //textual data
	static const DataFormat uriList; //a list of uri objects
	static const DataFormat image; //raw image data

public:
	/// The primary default name of the DataFormat.
	/// This will be used to compare multiple DataFormats and must not be empty, otherwise
	/// the DataFormat is invalid.
	/// This should be a mime-type, but does not have to be a standardized one if there is none.
	std::string name {};

	/// Additional names that this format might be recognized under. Basically a help for
	/// other applications that might know it the same format under a different name.
	/// More significant names/descriptions should come first. Can also contains none mime-type
	/// names, but should be avoided.
	std::vector<std::string> additionalNames {};
};

inline bool operator==(const DataFormat& a, const DataFormat& b) { return a.name == b.name; }
inline bool operator!=(const DataFormat& a, const DataFormat& b) { return !(a == b); }

/// The DataSource class is an interface implemented by the application to start drag and drop
/// actions or copy data into the clipboard.
/// The interface gives information about in which formats data can be represented and then
/// provides the data for a given format.
class DataSource {
public:
	virtual ~DataSource() = default;

	/// Returns all supported dataTypes format constants as a DataTypes object.
	virtual std::vector<DataFormat> formats() const = 0;

	/// Returns data in the given format.
	/// The std::any must contain a object of the type specified for the requested DataFormat.
	/// If retrieving data fails (because e.g. the requestes format is invalid) an
	/// empty std::any object should be returned.
	virtual std::any data(const DataFormat& format) const = 0;

	/// Returns an image representing the data. This image could e.g. used
	/// when this DataSource is used for a drag and drop opertation.
	/// If the data cannot be represented using an image, return a default-constructed
	/// Image object (or just don't override it).
	virtual Image image() const { return {}; };
};

/// Class that allows app to retrieve data from other apps
/// The DataOffer interface is usually implemented by the backends and will be passed to the
/// application either as result from a clipboard request or with a DataOfferEvent, if there
/// was data dropped onto a window.
/// It can then be used to determine the different data types in which the data can be represented
/// or to retrieve the data in a supported format.
/// On Destruction, the DataOffer should trigger all waiting data callback without data to
/// signal them that they dont have to wait for it any longer since retrieval failed.
class DataOffer {
public:
	using FormatsRequest = std::unique_ptr<AsyncRequest<std::vector<DataFormat>>>;
	using DataRequest = std::unique_ptr<AsyncRequest<std::any>>;

public:
	DataOffer() = default;
	virtual ~DataOffer() = default;

	/// Requests all supported formats in which the data is offered.
	/// On failure nullptr or a FormatsRequest without any formats should be returned.
	virtual FormatsRequest formats() = 0;

	/// Requests the offered data in the given DataFormat.
	/// If this failed because the format is not supported or the data cannot be retrieved,
	/// nullptr (if known at return time) or a DataRequest with en empty any object should
	/// be returned.
	virtual DataRequest data(const DataFormat&) = 0;
};

using DataOfferPtr = std::unique_ptr<DataOffer>;

std::vector<uint8_t> serialize(const Image&);
UniqueImage deserializeImage(nytl::Span<const uint8_t> buffer);

/// Encodes a vector of uris to a single string with mime-type text/uri-list encoded in utf8.
/// Will replace special chars with their escape codes and seperate the given uris using
/// newlines.
/// \sa decodeUriList
std::string encodeUriList(const std::vector<std::string>& uris);

/// Decodes a given utf8 encoded string of mime-type text/uri-list to a vector of uris.
/// Will replace '%' escape codes in the list with utf8 special chars and ignore comment lines.
/// \param removeComments removes uri lines that start with a '#'
/// \sa encodeUriList
std::vector<std::string> decodeUriList(const std::string& list, bool removeComments = true);

/// Returns a std::any that wraps the data of a raw buffer in the correct format
/// for the given parameters. Does basically check for standard formats and wrap the
/// raw buffer otherwise.
std::any wrap(std::vector<uint8_t> rawBuffer, const DataFormat& format);

/// Returns a raw buffer for the given std::any and the DataFormat for the data the any wraps.
std::vector<uint8_t> unwrap(std::any any, const DataFormat& format);

/// Checks whether the given format string matches the given DataFormat, i.e. if it one
/// of the descriptions/names of dataFormat.
bool match(const DataFormat& dataFormat, nytl::StringParam formatName);
bool match(const DataFormat& a, const DataFormat& b);

// TODO: for additional parameter (e.g. charset) parsing?
// TODO: text/plain might have other charsets then utf8 that should be handled as well...
// std::any wrap(nytl::Span<uint8_t> rawBuffer, nytl::StringParam formatName);
// std::vector<uint8_t> unwrap(const std::any& any, nytl::StringParam formatName);

} // namespace ny

// hash specialization for ny::DataFormat
namespace std
{
	template<>
	struct hash<ny::DataFormat> {
		auto operator()(const ny::DataFormat& format) const noexcept
		{
			std::hash<std::string> hasher;
			return hasher(format.name);
		}
	};
}
