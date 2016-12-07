// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <nytl/vec.hpp>

#include <memory>
#include <vector>

//Be careful with formats and byte ordering!
//A good read, from which we use some terminology:
//https://en.wikipedia.org/wiki/RGBA_color_space

//TODO: document formats, ordering inside ny more clearly
//TODO: more ImageDataFormats needed? e.g. rgb565 or rgba4444 or rgba16161616?
//TODO: rename ImageData -> Image (ImageDataFormat -> ImageFormat or PixelFormat)

namespace ny
{

///The differents formats in which image data can be represented.
///If one needs a 32-bit rgb color format (i.e. with 8 bits unused), just use a
///rgba/argb format and ensure that all alpha values are set to 255.
///\sa imageDataFormatSize
///\sa ImageData
enum class ImageDataFormat : unsigned int
{
	none,

	rgba8888,
	argb8888,
	rgb888,

	abgr8888, //reverse rgba
	bgra8888, //reverse argb
	bgr888, //reverse rgb

	r8,
	g8,
	b8,
	a8,
};

///Used to pass loaded or created images to functions.
///Note that this class does explicitly not implement any functions for creating/loading/changing
///the image itself, it is only used to hold all information needed to correctly interpret
///a raw image data buffer.
///\tparam P The pointer type to used. Should be a type that can be used as std::uint8_t*.
///Might be const or a smart pointer.
///\sa ImageDataFormat
///\sa imageDataFormatSize
template<typename P>
class BasicImageData
{
public:
	///The raw data of the image. Must be a pointer-like object to at least stride * size.y bytes.
	///The format of the data is specified by format. The data is layed out in endian-native
	///i.e. word order. This means that e.g. on a x86 (little-endian) machine, the first
	///byte of a pixel with rgba format would be the alpha (not the red!) byte.
	P data {};

	///The size of the represented image.
	nytl::Vec2ui size {};

	///The format of the raw image data in native-endian order.
	ImageDataFormat format {};

	///The size in bytes of one image data row.
	///Might be different from the packed size (size.x * imageDataFormatSize(format))
	///due to row alignment requirements (i.e. align the stride to 32 bit for SSE operatoins).
	///If the stride is 0, it should always be treated as the packed size.
	unsigned int stride {};
};

///BasicImageData specialization for owned image data that makes it copyable.
template<>
class BasicImageData<std::unique_ptr<uint8_t[]>>
{
public:
	std::unique_ptr<uint8_t[]> data {};
	nytl::Vec2ui size {};
	ImageDataFormat format {};
	unsigned int stride {};

public:
	BasicImageData() = default;
	~BasicImageData() = default;

	BasicImageData(const BasicImageData& other);
	BasicImageData& operator=(const BasicImageData& other);
};

using ImageData = BasicImageData<const uint8_t*>;
using MutableImageData = BasicImageData<uint8_t*>;
using OwnedImageData = BasicImageData<std::unique_ptr<uint8_t[]>>;

///Represents an animated image, i.e. a collection of imageDatas with a stored delay between each
///other. Usually all stored images in the collection should have the same format, size and stride.
///A vector holding the collection of imageDatas as well as the delay in milliseconds
///until the next image should be displayed.
///This typedef is intended as way to pass animated images (i.e. for cursors or icons) around that
///were loaded/created/manipulated using another toolkit.
///\sa ImageData
template<typename P>
using BasicAnimatedImageData = std::vector<std::pair<BasicImageData<P>, unsigned int>>;


//TODO: use bits!
///Returns the size of the given format in bytes.
///E.g. Format::rgba8888 would return 4, since one pixel of this format needs 4 bytes to
///be stored. The size will always be rounded up to bytes, e.g. Format::a8 returns 1.
///\sa ImageDataFormat
unsigned int imageDataFormatSize(ImageDataFormat);


///Converts between byte order (i.e. endian-independent) and word-order (endian-dependent)
///and vice versa. On a big-endian machine this function will simply return a format, but
///on a little-endian machine it will reverse the order of the format.
///Note that ImageData objects always hold data in word order, so if one works with a library
///that uses byte-order (as some e.g. image libraries do) they have to either convert the
///imageData data or the imageData format when passing/receiving from/to this library.
ImageDataFormat toggleWordByte(ImageDataFormat);

///Can be used to convert image data to another format or to change its stride alignment.
///\param alignNewStride Can be used to pass a alignment requirement for the stride of the
///new (converted) data. Defaulted to 0, in which case the packed size will be used as stride.
///\sa BasicImageData
///\sa ImageDataFormat
OwnedImageData convertFormat(const ImageData& fromImg, ImageDataFormat to,
	unsigned int alignNewStride = 0);

void convertFormat(const ImageData& fromImg, ImageDataFormat to, std::uint8_t& toData,
	unsigned int alignNewStride = 0);


///Returns the size in bytes of the given BasicImageData.
///\sa BasicImageData
template<typename P>
unsigned int dataSize(const BasicImageData<P>& imageData)
{
	auto stride = imageData.stride;
	if(!stride) stride = imageDataFormatSize(imageData.format) * imageData.size.x;
	return stride * imageData.size.y;
}

//Returns whether an ImageData object satisfied the given requirements.
//Returns false if the stride of the given ImageData satisfies the given align but
//is not as small as possible.
bool satisfiesRequirements(const ImageData&, ImageDataFormat format, unsigned int strideAlign = 0);

///Returns the color of the image at at the given position.
///Does not perform any range checking, i.e. if position lies outside of the size
///of the passed ImageData object, this call will result in undefined behavior.
nytl::Vec4u8 readPixel(const ImageData&, nytl::Vec2ui position);

///Sets the color of the pixel at the given position.
///Does not perform any range checking, i.e. if position lies outside of the size
///of the passed ImageData object, this call will result in undefined behavior.
void writePixel(const MutableImageData&, nytl::Vec2ui position, nytl::Vec4u8 color);

}
