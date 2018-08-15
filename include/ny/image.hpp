// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <nytl/vec.hpp> // nytl::Vec
#include <nytl/tmpUtil.hpp> // nytl::templatize
#include <memory> // std::unique_ptr
#include <cstring> // std::memcpy
#include <array> // std::array
#include <cmath> // std::ceil

namespace ny {

// NOTE: Before adding additional image formats, make sure to read the
// implementation note at the top of image.cpp

/// The differents formats in which image data can be represented.
/// If one needs a 32-bit rgb color format (i.e. with 8 bits unused), just use a
/// rgba/argb format and ensure that all alpha values are set to 255.
/// \sa imageDataFormatSize
/// \sa ImageData
enum class ImageFormat : unsigned int {
	none,

	rgba8888,
	argb8888,
	rgb888,

	abgr8888, // reverse rgba
	bgra8888, // reverse argb
	bgr888, // reverse rgb

	a8, // 8-bit alpha
	a1 // 1-bit alpha
};

/// Returns whether the current machine is little endian.
/// If this returns false it is assumed to be big endian.
bool littleEndian();

/// Returns the next multiple of alignment that is greater or equal than value.
/// Can be used to 'align' a value e.g. align(27, 8) returns 32.
template<typename A, typename B>
constexpr auto align(A x, B xalign) {
	return xalign ? std::ceil(x / double(xalign)) * xalign : x;
}

/// Returns the number of bits needed to store one pixel in the given format.
/// Example: bitSize(ImageFormat::bgr888) returns 24.
unsigned int bitSize(ImageFormat format);

/// Returns the number of bytes needed to store one pixel in the given format.
/// Sine the exact value might not be a multiple of one byte, this value is rouneded up.
/// Example: byteSize(ImageFormat::rgba8888) returns 4.
unsigned int byteSize(ImageFormat format);

/// Converts between byte order (i.e. endian-independent) and
/// word-order (endian-dependent) and vice versa.
/// On a big-endian machine this function will simply return the given format,
/// but on a little-endian machine it will reverse the order of the color
/// channels. Note that BasicImage objects always hold data in word order, so
/// if one works with a library that uses byte-order (as some image libraries
/// do) they have to either convert the data or the format when passing
/// to/receiving from this library.
/// Example: bitSize(ImageFormat::bgr888) returns
///  - on little endian: ImageFormat::rgb888 (reversed).
///  - on big endian: ImageFormat::bgr888 (not changed).
ImageFormat toggleByteWordOrder(const ImageFormat& format);

namespace detail {

template<typename T, typename F>
constexpr void copy(T& to, F from, unsigned int) {
	to = from;
}

template<typename T, typename PF>
void copy(T& to, const std::unique_ptr<PF[]>& from, unsigned int) {
	to = from.get();
}

template<typename PT>
void copy(std::unique_ptr<PT[]>& to, const uint8_t* from, unsigned int size) {
	if(!from) {
		to = {};
		return;
	}

	to = std::make_unique<PT[]>(size);
	std::memcpy(to.get(), from, size);
}

template<typename PT, typename PF>
void copy(std::unique_ptr<PT[]>& to, const std::unique_ptr<PF[]>& from,
		unsigned int size) {
	if(!from) {
		to = {};
		return;
	}

	to = std::make_unique<PT[]>(size);
	std::memcpy(to.get(), from.get(), size);
}

} // namespace detail

template<typename P> class BasicImage;

/// Returns the raw data from the given BasicImage as uint8_t pointer.
/// Useful for generic code since it allows to access the raw data
/// independently from the medium used to store it (e.g. be a smart pointer).
template<typename P>
constexpr auto data(const BasicImage<P>& img) {
	if constexpr(std::is_convertible_v<P, const uint8_t*>) {
		return img.data;
	} else if constexpr(std::is_convertible_v<decltype(&*img.data), const uint8_t*>) {
		return &*img.data;
	} else if constexpr(nytl::templatize<P>(true)) {
		static_assert("Invalid img data pointer");
	}
}

template<typename P> constexpr unsigned int dataSize(const BasicImage<P>& img);
template<typename P> constexpr unsigned int bitStride(const BasicImage<P>& img);

/// Represents the raw data of an image as well infomrmation to interpret it.
/// Note that this class does explicitly not implement any functions for creating/loading/changing
/// the image itself, it is only used to hold all information needed to correctly interpret
/// a raw image data buffer, generic functions for it can be created freely.
/// Its stride is stored in bits and therefore the image stride might not be a multiple
/// of 8 bits (1 byte).
/// The format it holds is always specified in word order, that means if an image with
/// a pixel 0xAABBCCDD has format rgba8888, this pixel will be interpreted as
/// rgba(0xAA, 0xBB, 0xCC, 0xDD) independent from endianess (note that on little endian
/// this is not how it is layed out in memory).
/// There are several helper functions that make dealing with BasicImage objects easier.
/// \tparam P The pointer type to used. Should be a type that can be used as std::uint8_t*.
/// Might be a cv-qualified or smart pointer.
template<typename P>
class BasicImage {
public:
	P data {}; // raw image data. References at least std::ceil(stride * size.y / 8.0) bytes
	nytl::Vec2ui size {}; // image size in pixels
	ImageFormat format {}; // data format in word order (endian-native)
	unsigned int stride {}; // stride in bits. At least size[0] * bitSize(format)

public:
	constexpr BasicImage() = default;
	~BasicImage() = default;

	constexpr BasicImage(P xdata, nytl::Vec2ui xsize, const ImageFormat& fmt, unsigned int strd = 0)
		: data(std::move(xdata)), size(xsize), format(fmt), stride(strd)
		{ if(!stride) stride = size[0] * bitSize(format); }

	template<typename O>
	constexpr BasicImage(const BasicImage<O>& lhs)
		: size(lhs.size), format(lhs.format), stride(bitStride(lhs))
		{ detail::copy(data, lhs.data, dataSize(lhs)); }

	template<typename O>
	constexpr BasicImage<P>& operator=(const BasicImage<O>& lhs) {
		size = lhs.size;
		format = lhs.format;
		stride = bitStride(lhs);
		detail::copy(data, lhs.data, dataSize(lhs));
		return *this;
	}

	constexpr BasicImage(const BasicImage& lhs)
		: size(lhs.size), format(lhs.format), stride(bitStride(lhs))
		{ detail::copy(data, lhs.data, dataSize(lhs)); }

	constexpr BasicImage& operator=(const BasicImage& lhs) {
		size = lhs.size;
		format = lhs.format;
		stride = bitStride(lhs);
		detail::copy(data, lhs.data, dataSize(lhs));
		return *this;
	}

	constexpr BasicImage(BasicImage&&) noexcept = default;
	constexpr BasicImage& operator=(BasicImage&&) noexcept = default;
};

using Image = BasicImage<const uint8_t*>; /// Default, immutable, non-owned BasicImgae typedef.
using MutableImage = BasicImage<uint8_t*>; /// Mutable, non-owned BasicImage typedef
using UniqueImage = BasicImage<std::unique_ptr<uint8_t[]>>; /// Mutable, owned BasicImage typedef
using SharedImage = BasicImage<std::shared_ptr<uint8_t[]>>; /// Mutable, shared BasicImage typedef

/// Returns the stride of the given BasicImage in bits.
/// If the given image has no stride stored, calculates the stride.
template<typename P>
constexpr unsigned int bitStride(const BasicImage<P>& img)
	{ return img.stride ? img.stride : img.size[0] * bitSize(img.format); }

/// Returns the stride of the given BasicImage in bytes (rounded up).
/// If the given image has no stride stored, calculates the stride.
template<typename P>
constexpr unsigned int byteStride(const BasicImage<P>& img)
	{ return img.stride ? std::ceil(img.stride / 8) : img.size[0] * byteSize(img.format); }

/// Returns the total amount of bytes the image data holds (rounded up).
template<typename P>
constexpr unsigned int dataSize(const BasicImage<P>& img)
	{ return std::ceil(bitStride(img) * img.size[1] / 8.0); }

/// Returns the bit of the given Image at which the pixel for the given position begins.
unsigned int pixelBit(const Image&, nytl::Vec2ui position);

/// Returns the color of the image at at the given position.
/// The returned vector will hold the components of the colorspace of the images format.
/// Does not perform any range checking, i.e. if position lies outside of the size
/// of the passed ImageData object, this call will result in undefined behavior.
nytl::Vec4u8 readPixel(const Image&, nytl::Vec2ui position);

/// Returns the color of the given pixel value for the given ImageFormat.
/// Results in undefined behaviour if the given format is invalid or the data referenced
/// by pixel does not have enough bytes to read.
/// \param bitOffset The bit position at which reading should start in signifance.
/// E.g. if bitOffset is 5, we should start reading the 5th most significant bit,
/// and then getting less significant by continuing with the 6th.
/// For formats whose bitSize is a multiple of 8, bitOffset must be 0.
nytl::Vec4u8 readPixel(const uint8_t& pixel, ImageFormat format, unsigned int bitOffset = 0u);

/// Sets the color of the pixel at the given position.
/// Does not perform any range checking, i.e. if position lies outside of the size
/// of the passed ImageData object, this call will result in undefined behavior.
void writePixel(const MutableImage&, nytl::Vec2ui position, nytl::Vec4u8 color);

/// Sets the color of the given pixel.
/// Results in undefined behaviour if the given format is invalid or the data referenced
/// by pixel does not have enough bytes to read.
/// \param bitOffset The bit position at which reading should start in significance.
/// For formats whose bitSize is a multiple of 8, bitOffset must be 0.
void writePixel(uint8_t& pixel, ImageFormat format, nytl::Vec4u8 color,
	unsigned int bitOffset = 0u);

/// Normalizes the given color values for the given format (color channel sizes).
/// Example: norm({255, 128, 511, 0}, rgba8888) returns {1.0, 0.5, 2.0, 0.0}
nytl::Vec4f norm(nytl::Vec4u64 color, ImageFormat format);

/// Returns whether an ImageData object satisfied the given requirements.
/// Returns false if the stride of the given ImageData satisfies the given align but
/// is not as small as possible.
/// \param strideAlign The required alignment of the stride in bits
bool satisfiesRequirements(const Image&, ImageFormat, unsigned int strideAlign = 0);

/// Can be used to convert image data to another format or to change its stride alignment.
/// \param alignNewStride Can be used to pass a alignment requirement for the stride of the
/// new (converted) data. Defaulted to 0, in which case the packed size will be used as stride.
/// \sa BasicImageData
/// \sa ImageDataFormat
UniqueImage convertFormat(const Image&, ImageFormat to, unsigned int alignNewStride = 0);
void convertFormat(const Image&, ImageFormat to, uint8_t& into, unsigned int alignNewStride = 0);

/// Returns whether the given format has an alpha component.
/// Despite the name, this will return false for the a1 and a8 image formats.
bool alphaComponent(ImageFormat);

/// Premutliplies the alpha factors for the given image.
/// Has no effect if the given image has no alpha channel or only an alpha channel.
/// \param resetAlpha Sets all alpha values to zero why premultiplying if true.
void premultiply(const MutableImage& img, bool resetAlpha = false);

} // namespace nytl
