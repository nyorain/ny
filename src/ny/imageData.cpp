// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/imageData.hpp>
#include <ny/log.hpp>
#include <cstring>

namespace ny
{

//TODO: expose/use util functions in public ImageData interface
//TODO: throw in places that will never be reached?

namespace
{

//LittleEndian: least significant byte first.
constexpr bool littleEndian()
{
	constexpr uint32_t dummyEndianTest = 0x1;
	return (((std::uint8_t*)&dummyEndianTest)[0] == 1);
}

unsigned int stride(const ImageData& img)
{
	auto ret = img.stride;
	if(!ret) ret = img.size.x * imageDataFormatSize(img.format);
	return ret;
}

void writePixel(std::uint8_t& pixel, ImageDataFormat format, const nytl::Vec4u8& color)
{
	using Format = ImageDataFormat;
	auto pixelSize = imageDataFormatSize(format);

	//The pixels bytes by significance
	//l1 always points to the most significant byte, while l2-l4 (depending on pixelSize)
	//points to the least significant byte belonging to the pixel
	//e.g. when wanting to write logical rgb 0xAABBCC: *b[1] = 0xAA, *b[2] = 0xBB, *b[3] = 0xCC.
	uint8_t* b[4] {};

	if(littleEndian())
	{
		if(pixelSize > 0) b[0] = &pixel + pixelSize - 1;
		if(pixelSize > 1) b[1] = &pixel + pixelSize - 2;
		if(pixelSize > 2) b[2] = &pixel + pixelSize - 3;
		if(pixelSize > 3) b[3] = &pixel + pixelSize - 4;
	}
	else
	{
		if(pixelSize > 0) b[0] = &pixel + 1;
		if(pixelSize > 1) b[1] = &pixel + 2;
		if(pixelSize > 2) b[2] = &pixel + 3;
		if(pixelSize > 3) b[3] = &pixel + 4;
	}

	switch(format)
	{
		case Format::bgra8888:
			*b[3] = color.w;
		case Format::bgr888:
			*b[0] = color.z;
			*b[1] = color.y;
			*b[2] = color.x;
			break;

		case Format::rgba8888:
			*b[3] = color.w;
		case Format::rgb888:
			*b[0] = color.x;
			*b[1] = color.y;
			*b[2] = color.z;
			break;

		case Format::argb8888:
			*b[0] = color.w;
			*b[1] = color.x;
			*b[2] = color.y;
			*b[3] = color.z;
			break;

		case Format::abgr8888:
			*b[0] = color.w;
			*b[1] = color.z;
			*b[2] = color.y;
			*b[3] = color.x;
			break;

		case Format::r8: *b[0] = color.x; break;
		case Format::g8: *b[0] = color.y; break;
		case Format::b8: *b[0] = color.z; break;
		case Format::a8: *b[0] = color.w; break;

		case Format::none: break;
	}

	//This should never be reached
}

nytl::Vec4u8 readPixel(const std::uint8_t& pixel, ImageDataFormat format)
{
	using Format = ImageDataFormat;
	auto pixelSize = imageDataFormatSize(format);
	auto bytes = &pixel;

	//the logical values, l1 is always the most significant byte read
	//e.g. (pixelSize = 4) 0xAABBCCDD: l1 = 0xAA, l4 = 0xDD
	//e.g. (pixelSize = 1) 0xAA: l1 = 0xAA, l2, l3, l4 = 0x00
	uint8_t l1, l2, l3, l4;

	if(littleEndian())
	{
		if(pixelSize > 0) l1 = bytes[pixelSize - 1];
		if(pixelSize > 1) l2 = bytes[pixelSize - 2];
		if(pixelSize > 2) l3 = bytes[pixelSize - 3];
		if(pixelSize > 3) l4 = bytes[pixelSize - 4];
	}
	else
	{
		if(pixelSize > 0) l1 = bytes[0];
		if(pixelSize > 1) l2 = bytes[1];
		if(pixelSize > 2) l3 = bytes[2];
		if(pixelSize > 3) l4 = bytes[3];
	}

	switch(format)
	{
		case Format::rgba8888: return {l1, l2, l3, l4};
		case Format::argb8888: return {l2, l3, l4, l1};
		case Format::abgr8888: return {l4, l3, l2, l1};
		case Format::bgra8888: return {l3, l2, l1, l4};
		case Format::rgb888: return {l1, l2, l3, 0};
		case Format::bgr888: return {l3, l2, l1, 0};
		case Format::r8: return {l1, 0, 0, 0};
		case Format::g8: return {0, l1, 0, 0};
		case Format::b8: return {0, 0, l1, 0};
		case Format::a8: return {0, 0, 0, l1};
		case Format::none: return {};
	}

	return {}; //should never be reached
}

} //anonymous namespace

unsigned int imageDataFormatSize(ImageDataFormat f)
{
	using Format = ImageDataFormat;
	switch(f)
	{
		case Format::rgba8888:
		case Format::argb8888:
		case Format::abgr8888:
		case Format::bgra8888:
			return 4u;

		case Format::rgb888:
		case Format::bgr888:
			return 3u;

		case Format::r8:
		case Format::g8:
		case Format::b8:
		case Format::a8:
			return 1u;

		case Format::none: return 0u;
	}

	return 0u; //should never be reached
}

ImageDataFormat byteToWordOrder(ImageDataFormat format)
{
	if(!littleEndian()) return format;

	using Format = ImageDataFormat;
	switch(format)
	{
		case Format::rgba8888: return Format::abgr8888;
		case Format::argb8888: return Format::bgra8888;
		case Format::abgr8888: return Format::rgba8888;
		case Format::bgra8888: return Format::argb8888;
		case Format::rgb888: return Format::bgr888;
		case Format::bgr888: return Format::rgb888;

		case Format::r8:
		case Format::g8:
		case Format::b8:
		case Format::a8:
		case Format::none: return format;
	}

	return format; //should never be reached
}

void convertFormat(const ImageData& img, ImageDataFormat to, std::uint8_t& toData,
	unsigned int alignNewStride)
{
	if(satisfiesRequirements(img, to, alignNewStride))
	{
		std::memcpy(&toData, img.data, dataSize(img));
		return;
	}

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

			auto color = readPixel((img.data)[oldpos], img.format);
			writePixel((&toData)[newpos], to, color);
		}
	}
}

OwnedImageData convertFormat(const ImageData& img, ImageDataFormat to,
	unsigned int alignNewStride)
{
	auto newStride = img.size.x * imageDataFormatSize(to);
	if(alignNewStride) newStride = std::ceil(newStride / alignNewStride) * alignNewStride;

	OwnedImageData ret;
	ret.data = std::make_unique<std::uint8_t[]>(newStride * img.size.y);
	ret.size = img.size;
	ret.format = to;
	ret.stride = newStride;
	convertFormat(img, to, *ret.data.get(), alignNewStride);

	return ret;
}

bool satisfiesRequirements(const ImageData& img, ImageDataFormat format, unsigned int strideAlign)
{
	auto smallestStride = img.size.x * imageDataFormatSize(format);
	if(strideAlign) smallestStride = std::ceil(smallestStride / strideAlign) * strideAlign;
	return (img.format == format && stride(img) == smallestStride);
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
