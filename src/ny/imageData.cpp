// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/imageData.hpp>
#include <ny/log.hpp>
#include <cstring>

namespace ny
{

// namespace
// {

//LittleEndian: least significant byte first.
constexpr bool littleEndian()
{
	constexpr uint32_t dummyEndianTest = 0x1;
	return (((std::uint8_t*)&dummyEndianTest)[0] == 1);
}

void convert(std::uint8_t& writeData, ImageDataFormat newformat, const nytl::Vec4u8& color)
{
	// using Format = ImageDataFormat;
	// std::uint32_t logicalValue {};
	//
	// switch(newformat)
	// {
	// 	case Format::bgra8888:
	// 		logicalValue = (color.w << 0);
	// 	case Format::bgr888:
	// 		logicalValue |= (color.x << 8);
	// 		logicalValue |= (color.y << 16);
	// 		logicalValue |= (color.z << 24);
	// 		break;
	//
	// 	case Format::rgba8888:
	// 		logicalValue = (color.w << 0);
	// 	case Format::rgb888:
	// 		logicalValue |= (color.x << 24);
	// 		logicalValue |= (color.y << 16);
	// 		logicalValue |= (color.z << 8);
	// 		break;
	//
	// 	case Format::argb8888:
	// 		logicalValue |= (color.w << 24);
	// 		logicalValue |= (color.x << 16);
	// 		logicalValue |= (color.y << 8);
	// 		logicalValue |= (color.z << 0);
	// 		break;
	//
	// 	case Format::a8:
	// 		logicalValue |= (color.w << 24);
	// 		break;
	//
	// 	default:
	// 		break;
	// }
	//
	// auto size = imageDataFormatSize(newformat);
	// std::memcpy(&writeData, &logicalValue,
}

nytl::Vec4u8 formatDataColor(const std::uint8_t& pixel, ImageDataFormat format)
{
	using Format = ImageDataFormat;
	auto pixelSize = imageDataFormatSize(format);
	if(!pixelSize) return {};


	//logical 32-bit color value
	//i.e. 0xFF00FFFF for rgba(FF, 0, FF, FF). or 0xFF for a(FF)
	//rgba: 0xRRGGBBAA
	//rgb: 0xRRGGBB = 0x00RRGGBB
	//a: 0xAA = 0x000000AA
	uint32_t logical {};
	auto* logical8 = reinterpret_cast<uint8_t*>(&logical);

	//load the logical value depending on the size we can read into memory
	//problem is that we cannot simply read a 32 bit value since the color memory size
	//might be e.g. 8 or 24 bit. Therefore we load it byte-per-byte as far as we can.
	// if(littleEndian())
	// {
	// 	if(pixelSize > 0) std::memcpy(logical8 + 0, &pixel + 3, 1);
	// 	if(pixelSize > 1) std::memcpy(logical8 + 1, &pixel + 2, 1);
	// 	if(pixelSize > 2) std::memcpy(logical8 + 2, &pixel + 1, 1);
	// 	if(pixelSize > 3) std::memcpy(logical8 + 3, &pixel + 0, 1);
	// }
	// else
	// {
		if(pixelSize > 0) std::memcpy(logical8 + 0, &pixel + 0, 1);
		if(pixelSize > 1) std::memcpy(logical8 + 1, &pixel + 1, 1);
		if(pixelSize > 2) std::memcpy(logical8 + 2, &pixel + 2, 1);
		if(pixelSize > 3) std::memcpy(logical8 + 3, &pixel + 3, 1);
	// }

	// example for rgba8888:
	// 0x RR GG BB AA  <-- logical number
	//    l1 l2 l3 l4

	// example for a8:
	// 0xAA = 0x 00 00 00 AA  <-- logical number
	//           l1 l2 l3 l4

	//the logical values, l4 is always the least significant byte read
	//depending on the format size is one of the bytes before it the most significant
	uint8_t l1 = (logical >> 24) & 0x000000FF;
	uint8_t l2 = (logical >> 16) & 0x000000FF;
	uint8_t l3 = (logical >> 8) & 0x000000FF;
	uint8_t l4 = (logical >> 0) & 0x000000FF;

	debug(std::hex, logical);

	switch(format)
	{
		case Format::bgra8888: return {l3, l2, l1, l4};
		case Format::argb8888: return {l2, l3, l4, l1};
		case Format::rgba8888: return {l1, l2, l3, l4};
		case Format::rgb888: return {l2, l3, l4, 0};
		case Format::bgr888: return {l4, l3, l2, 0};
		case Format::a8: return {0, 0, 0, l4};
		default: return {};
	}
}

// } //anonymous namespace

unsigned int imageDataFormatSize(ImageDataFormat f)
{
	using Format = ImageDataFormat;
	switch(f)
	{
		case Format::rgba8888: case Format::argb8888: case Format::bgra8888: return 4;
		case Format::rgb888: case Format::bgr888: return 3;
		case Format::a8: return 1;
		default: return 0;
	}
}

void convertFormat(const ImageData& img, ImageDataFormat to, std::uint8_t& toData,
	unsigned int alignNewStride)
{
	auto newfs = imageDataFormatSize(to);
	auto oldfs = imageDataFormatSize(img.format);

	auto stride = img.stride;
	if(!stride) stride = oldfs * img.size.x;

	auto newStride = img.size.x * newfs;
	if(alignNewStride) newStride = std::ceil(newStride / alignNewStride) * alignNewStride;

	for(auto y = 0u; y < img.size.y; ++y)
	{
		for(auto x = 0u; x < img.size.x; ++x)
		{
			auto newpos = y * newStride + x * newfs;
			auto oldpos = y * stride + x * oldfs;

			auto color = formatDataColor((img.data)[oldpos], img.format);
			convert((&toData)[newpos], to, color);
		}
	}
}

std::unique_ptr<std::uint8_t[]> convertFormat(const ImageData& img, ImageDataFormat to,
	unsigned int alignNewStride)
{
	auto newStride = img.size.x * imageDataFormatSize(to);
	if(alignNewStride) newStride = std::ceil(newStride / alignNewStride) * alignNewStride;

	auto ret = std::make_unique<std::uint8_t[]>(newStride * img.size.y);
	convertFormat(img, to, *ret.get(), alignNewStride);
	return ret;
}

OwnedImageData::BasicImageData(const OwnedImageData& other)
	: size(other.size), format(other.format), stride(other.stride)
{
	auto size = dataSize(other);
	data = std::make_unique<uint8_t[]>(size);
	std::memcpy(data.get(), other.data.get(), size);
}

OwnedImageData& OwnedImageData::operator=(const OwnedImageData& other)
{
	size = other.size;
	format = other.format;
	stride = other.stride;

	auto size = dataSize(other);
	data = std::make_unique<uint8_t[]>(size);
	std::memcpy(data.get(), other.data.get(), size);

	return *this;
}

}
