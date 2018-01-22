// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/dataExchange.hpp>
#include <nytl/utf.hpp> // nytl::nth
#include <nytl/tmpUtil.hpp> // nytl::unused
#include <dlg/dlg.hpp>

#include <sstream> // std::ostringstream
#include <cstring> // std::memcpy

namespace ny {
namespace {

bool sameBeginning(const char* a, const char* b)
{
	if(!a || !b || *a == '\0' || *b == '\0') {
		return false;
	}

	return !std::strcmp(a, b);
}

} // anoymous util namespace

// default data formats
const DataFormat DataFormat::none {};
const DataFormat DataFormat::raw {"application/octet-stream",
	{"application/binary", "applicatoin/unknown", "raw", "binary", "buffer", "unknown"}};
const DataFormat DataFormat::text {"text/plain", {"text", "string",
	"unicode", "utf8", "STRING", "TEXT", "UTF8_STRING", "UNICODETEXT"}};
const DataFormat DataFormat::uriList {"text/uri-list", {"uriList"}};
const DataFormat DataFormat::image {"image/x-ny-data", {"ny::Image"}};

std::vector<uint8_t> serialize(const Image& image)
{
	std::vector<uint8_t> ret;

	// format
	ret.resize(2 * 4u + 4 + 4); // size, stride, format
	reinterpret_cast<uint32_t&>(ret[0]) = image.size[0];
	reinterpret_cast<uint32_t&>(ret[4]) = image.size[1];
	reinterpret_cast<uint32_t&>(ret[8]) = image.stride;
	reinterpret_cast<uint32_t&>(ret[12]) = static_cast<uint32_t>(image.format);

	// total size of data
	auto start = ret.size();
	auto dsize = dataSize(image);
	ret.resize(ret.size() + dsize);

	// data
	std::memcpy(&ret[start], image.data, dsize);

	return ret;
}

UniqueImage deserializeImage(nytl::Span<const uint8_t> buffer)
{
	UniqueImage image;
	auto headerSize = 2 * 4u + 4 + 4; // size, stride, format

	// check for invaliad heaer/empty data
	if(buffer.size() < headerSize) {
		dlg_warn("invalid serialized image header");
		return {};
	}

	image.size[0] = reinterpret_cast<const uint32_t&>(buffer[0]);
	image.size[1] = reinterpret_cast<const uint32_t&>(buffer[4]);
	image.stride = reinterpret_cast<const uint32_t&>(buffer[8]);
	image.format = static_cast<ImageFormat>(reinterpret_cast<const uint32_t&>(buffer[12]));

	auto dSize = dataSize(image);

	//check for invalid data size
	if(buffer.size() < dSize + headerSize) {
		dlg_warn("invalid serialized image data size");
		return {};
	}

	image.data = std::make_unique<uint8_t[]>(dSize);
	std::memcpy(image.data.get(), &buffer[headerSize], dSize);

	return image;
}

// see roughly: https://tools.ietf.org/html/rfc3986
std::string encodeUriList(const std::vector<std::string>& uris)
{
	std::string ret;
	ret.reserve(uris.size() * 10);

	// first put the uris together and escape special chars
	// correct utf8 parsing
	for(auto& uri : uris) {
		for(auto i = 0u; i < nytl::charCount(uri); ++i) {
			auto chars = nytl::nth(uri, i);
			auto cint = reinterpret_cast<uint32_t&>(*chars.data());

			// the chars that should not be encoded in uris (besides alphanum values)
			std::string special = ":/?#[]@!$&'()*+,;=-_~.";
			if(cint <= 255 && (std::isalnum(cint) || special.find(cint) != std::string::npos)) {
				ret.append(chars.data());
			} else {
				auto last = 0u;
				for(auto i = 0u; i < chars.size(); ++i) if(chars[i]) last = i;
				for(auto i = 0u; i <= last; ++i) {
					unsigned int ci = static_cast<unsigned char>(chars[i]);
					std::ostringstream sstream;
					sstream << std::hex << ci;
					ret.append("%");
					ret.append(sstream.str());
				}
			}
		}

		// note that the uri spec sperates lines with "\r\n"
		ret.append("\r\n");
	}

	return ret;
}

std::vector<std::string> decodeUriList(const std::string& escaped, bool removeComments)
{
	std::string uris;
	uris.reserve(escaped.size());

	// copy <escaped> into (non-const) <uris> into this loop, but replace
	// the escape codes on the run
	for(auto i = 0u; i < escaped.size(); ++i) {
		if(escaped[i] != '%') {
			uris.insert(uris.end(), escaped[i]);
			continue;
		}

		// invalid escape
		if(i + 2 >= escaped.size()) break;

		// % is always followed by 2 hexadecimal numbers
		char number[3] = {escaped[i + 1], escaped[i + 2], 0};
		auto num = std::strtol(number, nullptr, 16);

		// if we receive some invalid escape like "%yy" we will simply ignore it
		if(num) uris.insert(uris.end(), num);
		i += 2;
	}

	std::vector<std::string> ret;

	// split the list and check for comments if they should be removed
	// note that the uri spec sperates lines with "\r\n"
	while(true) {
		auto pos = uris.find("\r\n");
		if(pos == std::string::npos) break;

		auto uri = uris.substr(0, pos);
		if(!uri.empty() && ((uris[0] != '#') || !removeComments)) ret.push_back(std::move(uri));
		uris.erase(0, pos + 2);
	}

	return ret;
}

bool match(const DataFormat& dataFormat, const char* formatName)
{
	if(sameBeginning(dataFormat.name.c_str(), formatName)) return true;
	for(auto name : dataFormat.additionalNames)
		if(sameBeginning(name.c_str(), formatName)) return true;

	return false;
}

bool match(const DataFormat& a, const DataFormat& b)
{
	if(sameBeginning(a.name.c_str(), b.name.c_str())) return true;

	for(auto aname : a.additionalNames)
		for(auto bname : b.additionalNames)
			if(sameBeginning(aname.c_str(), bname.c_str())) 
				return true;

	return false;
}

std::any wrap(std::vector<uint8_t> buffer, const DataFormat& fmt)
{
	if(fmt == DataFormat::text) return std::string(buffer.begin(), buffer.end());
	if(fmt == DataFormat::uriList) return decodeUriList({buffer.begin(), buffer.end()});
	if(fmt == DataFormat::image) return  deserializeImage({buffer.data(), buffer.size()});

	return {std::move(buffer)};
}

std::vector<uint8_t> unwrap(std::any any, const DataFormat& format)
{
	if(format == DataFormat::text) {
		auto string = std::any_cast<const std::string&>(any);
		return {string.begin(), string.end()};
	} else if(format == DataFormat::uriList) {
		auto uris = std::any_cast<const std::vector<std::string>&>(any);
		auto string = encodeUriList(uris);
		return {string.begin(), string.end()};
	} else if(format == DataFormat::image) {
		auto img = std::any_cast<const UniqueImage&>(any);
		return serialize(img);
	}

	return std::move(std::any_cast<std::vector<uint8_t>&>(any));
}

} // namespace ny
