// Huge parts of this file (and the implementation) are basically just evg/image to keep
// independence from evg.

#pragma once

#include <ny/include.hpp>
#include <nytl/vec.hpp>
#include <memory>

namespace ny
{

enum class ImageDataFormat
{
	none,
	rgba8888,
	bgra8888,
	argb8888,
	rgb888,
	bgr888,
	a8
};

class ImageData
{
public:
	ImageData() = default;
	ImageData(const std::uint8_t& data, const nytl::Vec2ui& size, ImageDataFormat format);
	ImageData(const std::uint8_t& data, const nytl::Vec2ui& size, ImageDataFormat format, 
		unsigned int stride);

	~ImageData() = default;
	ImageData(const ImageData& other) noexcept = default;
	ImageData& operator=(const ImageData& other) noexcept = default;
	

public:
	const std::uint8_t* data;
	nytl::Vec2ui size;
	ImageDataFormat format;
	unsigned int stride;
};

///Returns the size of the given format in bytes.
///E.g. Format::rgba8888 would return 4, since one pixel of this format needs 4 bytes to
///be stored.
unsigned int imageDataFormatSize(ImageDataFormat f);

///Can be used to convert image data to another format.
///\param stride The stride of the given image data. Defaulted to 0, in which case
///the given size * the size of the given format will be used as stride.
///The returned data will be tightly packed (no row paddings).
std::unique_ptr<std::uint8_t[]> convertFormat(ImageDataFormat from, ImageDataFormat to,
	const std::uint8_t& data, const nytl::Vec2ui& size, unsigned int stride = 0, 
	unsigned int newStride = 0);

void convertFormat(ImageDataFormat from, ImageDataFormat to, const std::uint8_t& fromData,
	std::uint8_t& toData, const nytl::Vec2ui& size, unsigned int stride = 0, 
	unsigned int newStride = 0);

}
