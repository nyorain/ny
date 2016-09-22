#include <ny/base/imageData.hpp>
#include <ny/base/log.hpp>

namespace ny
{

namespace
{
	void convert(std::uint8_t& writeData, ImageDataFormat newformat, const nytl::Vec4uc& color)
	{
		using Format = ImageDataFormat;
		auto* newdata =  &writeData;

		switch(newformat)
		{
			case Format::bgra8888:
				newdata[3] = color.w;
			case Format::bgr888:
				newdata[0] = color.z;
				newdata[1] = color.y;
				newdata[2] = color.x;
				break;

			case Format::rgba8888:
				newdata[3] = color.w;
			case Format::rgb888:
				newdata[0] = color.x;
				newdata[1] = color.y;
				newdata[2] = color.z;
				break;

			case Format::argb8888:
				newdata[0] = color.w;
				newdata[1] = color.x;
				newdata[2] = color.y;
				newdata[3] = color.z;
				break;

			case Format::a8:
				newdata[0] = color.w;
				break;

			default:
				break;
		}
	}

	nytl::Vec4uc formatDataColor(const std::uint8_t& pixel, ImageDataFormat format)
	{
		using Format = ImageDataFormat;
		auto pixelSize = imageDataFormatSize(format);
		std::uint8_t d1, d2, d3, d4;

		if(pixelSize > 0) d1 = *(&pixel + 0);
		if(pixelSize > 1) d2 = *(&pixel + 1);
		if(pixelSize > 2) d3 = *(&pixel + 2);
		if(pixelSize > 3) d4 = *(&pixel + 3);

		switch(format)
		{
			case Format::bgra8888: return {d3, d2, d1, d4};
			case Format::argb8888: return {d4, d1, d2, d3};
			case Format::rgba8888: return {d1, d2, d3, d4};
			case Format::rgb888: return {d1, d2, d3, 0};
			case Format::bgr888: return {d3, d2, d1, 0};
			case Format::a8: return {0, 0, 0, d1};
			default: return {};
		}
	}
}

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

ImageData::ImageData(const std::uint8_t& data, const nytl::Vec2ui& size, ImageDataFormat format)
	: data(&data), size(size), format(format), stride(imageDataFormatSize(format) * size.x)
{
}

ImageData::ImageData(const std::uint8_t& data, const nytl::Vec2ui& size, ImageDataFormat format,
	unsigned int stride) : data(&data), size(size), format(format), stride(stride)
{
}

void convertFormat(ImageDataFormat from, ImageDataFormat to, const std::uint8_t& fromData,
	std::uint8_t& toData, const nytl::Vec2ui& size, unsigned int stride, 
	unsigned int alignNewStride)
{
	auto newfs = imageDataFormatSize(to);
	auto oldfs = imageDataFormatSize(from);

	if(!stride) stride = oldfs * size.x;

	auto newStride = size.x * newfs;
	if(alignNewStride) newStride = std::ceil(newStride / alignNewStride) * alignNewStride;

	for(auto y = 0u; y < size.y; ++y)
	{
		for(auto x = 0u; x < size.x; ++x)
		{
			auto newpos = y * newStride + x * newfs;
			auto oldpos = y * stride + x * oldfs;

			auto color = formatDataColor((&fromData)[oldpos], from);
			convert((&toData)[newpos], to, color);
		}
	}
}

std::unique_ptr<std::uint8_t[]> convertFormat(ImageDataFormat from, ImageDataFormat to,
	const std::uint8_t& data, const nytl::Vec2ui& size, unsigned int stride, 
	unsigned int alignNewStride)
{
	auto newStride = size.x * imageDataFormatSize(to);
	if(alignNewStride) newStride = std::ceil(newStride / alignNewStride) * alignNewStride;

	auto ret = std::make_unique<std::uint8_t[]>(newStride * size.y);
	convertFormat(from, to, data, *ret.get(), size, stride, alignNewStride);
	return ret;
}

}
