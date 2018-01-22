// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/image.hpp>

#include <array> // std::array
#include <cmath> // std::ceil

// NOTE on implementation:
// Due to the wanted simplicity of ny/image most of the functions here
// are implemented with hardcoded format switches.
// Most functions must be changed when additional formats are introduced.
// There exist a (rather complex) ny/image implementation (removed 02.05.2017) that
// implements the functions only by a meta-description of formats, which was removed
// due to being over-designed for the needs of ny, rather error-prone and not well tested.
//
// Functions to be changed when adding a new format include
// - toggleWordOrder
// - {bit,byte}Size
// - {read/write}Pixel (only the bitOffset versions)
//   - may need huge changed for not bit or byte aligned formats
// - norm
//
// When functions with a color precision higher than 8 bits are added, the
// parameters of all color taking or returning functions must be changed to a higher
// value (32 or 64 bits).

namespace ny {

bool littleEndian()
{
	// Runtime test checking for little endianess.
	constexpr uint32_t dummy = 1u;
	return ((reinterpret_cast<const uint8_t*>(&dummy))[0] == 1);
}

unsigned int bitSize(ImageFormat format)
{
	using Format = ImageFormat;
	switch(format) {
		case Format::rgba8888:
		case Format::argb8888:
		case Format::abgr8888:
		case Format::bgra8888:
			return 32u;

		case Format::rgb888:
		case Format::bgr888:
			return 24u;

		case Format::a8:
			return 8u;

		case Format::a1:
			return 1u;

		case Format::none: return 0u;
	}

	// should not be reached
	return 0u;
}

unsigned int byteSize(ImageFormat format)
{
	return std::ceil(bitSize(format) / 8.0);
}

ImageFormat toggleByteWordOrder(ImageFormat format)
{
	if(!littleEndian()) return format;

	using Format = ImageFormat;
	switch(format) {
		case Format::rgba8888: return Format::abgr8888;
		case Format::argb8888: return Format::bgra8888;
		case Format::abgr8888: return Format::rgba8888;
		case Format::bgra8888: return Format::argb8888;
		case Format::rgb888: return Format::bgr888;
		case Format::bgr888: return Format::rgb888;
		case Format::a8:
		case Format::a1:
		case Format::none: return format;
	}

	// should not be reached
	return Format::none;
}

unsigned int pixelBit(const Image& image, nytl::Vec2ui pos)
{
	return image.stride * pos[1] + bitSize(image.format) * pos[0];
}

nytl::Vec4u8 readPixel(const uint8_t& pixel, ImageFormat format, unsigned int bitOffset)
{
	unsigned int bytesToLoad = byteSize(format); // NOTE: must be changed for [2,7]-bit formats
	std::array<uint8_t, 4> bytes {};

	// logical word-order bytes
	bytes[0] = *(&pixel + (littleEndian() ? bytesToLoad - 1 : 0));
	if(bytesToLoad > 1) bytes[1] = *(&pixel + (littleEndian() ? bytesToLoad - 2 : 1));
	if(bytesToLoad > 2) bytes[2] = *(&pixel + (littleEndian() ? bytesToLoad - 3 : 2));
	if(bytesToLoad > 3) bytes[3] = *(&pixel + (littleEndian() ? bytesToLoad - 4 : 3));

	using Format = ImageFormat;
	switch(format) {
		case Format::rgba8888: return {bytes[0], bytes[1], bytes[2], bytes[3]};
		case Format::argb8888: return {bytes[1], bytes[2], bytes[3], bytes[0]};
		case Format::abgr8888: return {bytes[3], bytes[2], bytes[1], bytes[0]};
		case Format::bgra8888: return {bytes[2], bytes[1], bytes[0], bytes[3]};

		case Format::rgb888: return {bytes[0], bytes[1], bytes[2], 0};
		case Format::bgr888: return {bytes[2], bytes[1], bytes[0], 0};

		case Format::a8: return {0, 0, 0, bytes[0]};
		case Format::a1: return {0, 0, 0, static_cast<uint8_t>(bytes[0] & (1 << (8 - bitOffset)))};
		case Format::none: return {};
	}

	// should not be reached
	return {};
}

void writePixel(uint8_t& pixel, ImageFormat format, nytl::Vec4u8 color, unsigned int bitOffset)
{
	unsigned int bytesToWrite = byteSize(format); // NOTE: must be changed for [2,7]-bit formats
	std::array<uint8_t, 4> bytes {}; // word-order bytes to write

	using Format = ImageFormat;
	switch(format) {
		case Format::rgba8888: bytes = {color[0], color[1], color[2], color[3]}; break;
		case Format::argb8888: bytes = {color[3], color[0], color[1], color[2]}; break;
		case Format::abgr8888: bytes = {color[3], color[2], color[1], color[0]}; break;
		case Format::bgra8888: bytes = {color[2], color[1], color[0], color[3]}; break;

		case Format::rgb888: bytes = {color[0], color[1], color[2], 0}; break;
		case Format::bgr888: bytes = {color[2], color[1], color[0], 0}; break;

		case Format::a8: bytes[3] = color[4]; break;
		case Format::a1:
			bytes[0] = pixel & ~(1 << (8 - bitOffset));
		 	bytes[0] |= (color[3] & (1 << 8)) >> bitOffset;
			break;
		case Format::none: bytes = {};
	}

	*(&pixel + 0) = bytes[littleEndian() ? bytesToWrite - 1 : 0];
	if(bytesToWrite > 1) *(&pixel + 1) = bytes[littleEndian() ? bytesToWrite - 2 : 1];
	if(bytesToWrite > 2) *(&pixel + 2) = bytes[littleEndian() ? bytesToWrite - 3 : 2];
	if(bytesToWrite > 3) *(&pixel + 3) = bytes[littleEndian() ? bytesToWrite - 4 : 3];
}

nytl::Vec4u8 readPixel(const Image& img, nytl::Vec2ui pos)
{
	auto bit = pixelBit(img, pos);
	return readPixel(*(img.data + bit / 8), img.format, bit % 8);
}

void writePixel(const MutableImage& img, nytl::Vec2ui pos, nytl::Vec4u8 color)
{
	auto bit = pixelBit(img, pos);
	return writePixel(*(img.data + bit / 8), img.format, color, bit % 8);
}

nytl::Vec4f norm(nytl::Vec4u8 color, ImageFormat format)
{
	auto ret = static_cast<nytl::Vec4f>(color);

	if(format == ImageFormat::a1 || format == ImageFormat::none) return ret;
	return (1 / 255.f) * ret;
}

bool satisfiesRequirements(const Image& img, ImageFormat format, unsigned int strideAlign)
{
	auto smallestStride = img.size[0] * bitSize(format);
	if(strideAlign) smallestStride = align(smallestStride, strideAlign);
	return (img.format == format && bitStride(img) == smallestStride);
}

UniqueImage convertFormat(const Image& img, ImageFormat to, unsigned int alignNewStride)
{
	auto newStride = img.size[0] * bitSize(to);
	if(alignNewStride) newStride = align(newStride, alignNewStride);

	UniqueImage ret;
	ret.data = std::make_unique<std::uint8_t[]>(std::ceil((newStride * img.size[1]) / 8.0));
	ret.size = img.size;
	ret.format = to;
	ret.stride = newStride;
	convertFormat(img, to, *ret.data.get(), alignNewStride);

	return ret;
}

void convertFormat(const Image& img, ImageFormat to, uint8_t& into, unsigned int alignNewStride)
{
	if(satisfiesRequirements(img, to, alignNewStride)) {
		std::memcpy(&into, img.data, dataSize(img));
		return;
	}

	auto newStride = img.size[0] * bitSize(to);
	if(alignNewStride) newStride = align(newStride, alignNewStride);

	for(auto y = 0u; y < img.size[1]; ++y) {
		for(auto x = 0u; x < img.size[0]; ++x) {
			auto color = readPixel(img, {x, y});
			auto bit = y * newStride + x * bitSize(to);
			writePixel(*(&into + bit / 8), to, color, bit % 8);
		}
	}
}

bool alphaComponent(ImageFormat format)
{
	using Format = ImageFormat;
	switch(format) {
		case Format::argb8888:
		case Format::rgba8888:
		case Format::abgr8888:
		case Format::bgra8888:
			return true;

		case Format::rgb888:
		case Format::bgr888:
		case Format::a8:
		case Format::a1:
		case Format::none:
			return false;
	}

	// should not be reached
	return false;
}

void premultiply(const MutableImage& img, bool resetAlpha)
{
	if(!alphaComponent(img.format))
		return;

	for(auto y = 0u; y < img.size[1]; ++y) {
		for(auto x = 0u; x < img.size[0]; ++x) {
			auto color = readPixel(img, {x, y});
			auto alpha = norm(color, img.format)[3];
			color[0] *= alpha;
			color[1] *= alpha;
			color[2] *= alpha;
			if(resetAlpha) color[3] = 0;
			writePixel(img, {x, y}, color);
		}
	}
}

} // namespace ny
