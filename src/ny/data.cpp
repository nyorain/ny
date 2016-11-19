// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/data.hpp>
#include <ny/imageData.hpp>

namespace ny
{

namespace
{
	//TODO: make this extenable for custom data types

	const struct
	{
		unsigned int dataType;
		std::vector<const char*> mimes;
		std::vector<const char*> nonMimes;
	} mimeMappings[] =
	{
		{dataType::raw, {"x-application/ny-raw-buffer"}, {"buffer", "raw"}},
		{dataType::text,
			{ "text/plain", "text/plain;charset=utf8" },
			{ "string", "text", "STRING", "TEXT", "UTF8_STRING", "UNICODETEXT"}},
		{dataType::uriList, {"text/uri-list"}, {}},
		{dataType::image, {"image/x-ny-data"}, {"ny::ImageData"}},
		{dataType::timePoint,
			{"x-application/ny-time-point"},
			{"nytl::Timepoint", "std::chrono::high_resolution_clock::time_point"}},
		{dataType::timeDuration,
			{"x-applicatoin/ny-time-duration"},
			{"nytl::TimeDuration", "std::chrono::high_resolution_clock::duration"}},
		{dataType::bmp, {"image/bmp"}, {}},
		{dataType::png, {"image/png"}, {}},
		{dataType::jpeg, {"image/jpeg"}, {}},
		{dataType::gif, {"image/gif"}, {}},
		{dataType::mp3, {"image/mp3"}, {}},
		{dataType::mp4, {"image/mp4"}, {}},
		{dataType::webm, {"image/webm"}, {}}
	};
}

void DataTypes::add(unsigned int type)
{
	if(contains(type)) return;
	types.push_back(type);
}

void DataTypes::remove(unsigned int type)
{
    auto it = types.begin();
    while(it != types.end())
    {
        if(*it == type)
        {
           types.erase(it);
           return;
        }
        ++it;
    }
}

bool DataTypes::contains(unsigned int type) const
{
	for(auto t : types) if(t == type) return true;
    return false;
}

unsigned int stringToDataType(nytl::StringParam type, bool onlyMime)
{
	for(auto& mapping : mimeMappings)
	{
		for(auto mime : mapping.mimes) if(type == mime) return mapping.dataType;
		if(!onlyMime) for(auto nmime : mapping.nonMimes) if(type == nmime) return mapping.dataType;
	}

	return dataType::none;
}

std::vector<const char*> dataTypeToString(unsigned int type, bool onlyMime)
{
	for(auto& mapping : mimeMappings)
	{
		if(mapping.dataType == type)
		{
			std::vector<const char*> ret {mapping.mimes.begin(), mapping.mimes.end()};
			if(!onlyMime) ret.insert(ret.end(), mapping.nonMimes.begin(), mapping.nonMimes.end());
			return ret;
		}
	}

	return {};
}

std::vector<std::uint8_t> serialize(const ImageData& image)
{
	nytl::unused(image);
	return {};
}

OwnedImageData deserializeImageData(const std::vector<std::uint8_t>& buffer)
{
	nytl::unused(buffer);
	return {};
}

std::string encodeUriList(const std::vector<std::string>& uris)
{
	nytl::unused(uris);
	return {};
}

std::vector<std::string> decodeUriList(nytl::StringParam list)
{
	nytl::unused(list);
	return {};
}

}
