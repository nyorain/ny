// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <nytl/vec.hpp> // nytl::Vec
#include <memory> // std::unique_ptr
#include <cstring> // std::memcpy
#include <array> // std::array

// TODO: c++17 make ImageFormat pod struct derived from std::array and
//   the default formats constexpr members (i.e. ImageFormat::rgba8888)
//   make also the utility functions constexpr

namespace ny {

/// Represents a ColorChannel for an ImageFormat specification.
enum class ColorChannel : uint8_t {
	none,
	red,
	green,
	blue,
	alpha
};

/// An ImageFormat specifies the way the colors of an Image pixel is interpreted.
/// It specifies the different used color channels and their size in bits.
/// For offsets and spacings ColorChannel::none can be used. All other ColorChannels are allowed
/// to appear only once per format (e.g. there can't be 8 red bits then 8 green bits and
/// then 8 red bits again for one pixel).
/// Note that the size of one channel must not be larger than 64 bits since larger values
/// cannot be represented. Channels with a size of 0 should be ignored.
/// There is place for 9 channels because a format can have 4 "real" channels (r,g,b,a) and
/// then spacings (ColorChannel::none) between each of them in the beginning and end.
using ImageFormat = std::array<std::pair<ColorChannel, uint8_t>, 9>;

// - Default formats -
namespace imageFormats {

constexpr ImageFormat rgba8888 {{
	{ColorChannel::red, 8},
	{ColorChannel::green, 8},
	{ColorChannel::blue, 8},
	{ColorChannel::alpha, 8},
}};

constexpr ImageFormat abgr8888 {{
	{ColorChannel::alpha, 8},
	{ColorChannel::blue, 8},
	{ColorChannel::green, 8},
	{ColorChannel::red, 8},
}};

constexpr ImageFormat argb8888 {{
	{ColorChannel::alpha, 8},
	{ColorChannel::red, 8},
	{ColorChannel::green, 8},
	{ColorChannel::blue, 8},
}};

constexpr ImageFormat bgra8888 {{
	{ColorChannel::blue, 8},
	{ColorChannel::green, 8},
	{ColorChannel::red, 8},
	{ColorChannel::alpha, 8},
}};

constexpr ImageFormat bgr888 {{
	{ColorChannel::blue, 8},
	{ColorChannel::green, 8},
	{ColorChannel::red, 8},
}};

constexpr ImageFormat rgb888 {{
	{ColorChannel::red, 8},
	{ColorChannel::green, 8},
	{ColorChannel::blue, 8},
}};

constexpr ImageFormat xrgb8888 {{
	{ColorChannel::none, 8},
	{ColorChannel::red, 8},
	{ColorChannel::green, 8},
	{ColorChannel::blue, 8},
}};

constexpr ImageFormat a8 {{{ColorChannel::alpha, 8}}};
constexpr ImageFormat a1 {{{ColorChannel::alpha, 1}}};

constexpr ImageFormat r8 {{{ColorChannel::red, 8}}};
constexpr ImageFormat r1 {{{ColorChannel::red, 1}}};

constexpr ImageFormat g8 {{{ColorChannel::green, 8}}};
constexpr ImageFormat g1 {{{ColorChannel::green, 1}}};

constexpr ImageFormat b8 {{{ColorChannel::blue, 8}}};
constexpr ImageFormat b1 {{{ColorChannel::blue, 1}}};

constexpr ImageFormat none {};

}

/// Returns whether the current machine is little endian.
/// If this returns false it is assumed to be big endian.
bool littleEndian();

/// Returns the next multiple of alignment that is greater or equal than value.
/// Can be used to 'align' a value e.g. align(27, 8) returns 32.
template<typename A, typename B>
constexpr auto align(A value, B alignment)
	{ return alignment ? std::ceil(value / double(alignment)) * alignment : value; }

/// Returns the number of bits needed to store one pixel in the given format.
unsigned int bitSize(const ImageFormat& format);

/// Returns the number of bytes needed to store one pixel in the given format.
/// Sine the exact value might not be a multiple of one byte, this value is rouneded up.
/// Example: byteSize(ImageFormat::)
unsigned int byteSize(const ImageFormat& format);

/// Converts between byte order (i.e. endian-independent) and word-order (endian-dependent)
/// and vice versa. On a big-endian machine this function will simply return the given format, but
/// on a little-endian machine it will reverse the order of the color channels..
/// Note that BasicImage objects always hold data in word order, so if one works with a library
/// that uses byte-order (as some e.g. image libraries do) they have to either convert the
/// data or the format when passing/receiving from/to this library.
ImageFormat toggleByteWordOrder(const ImageFormat& format);

namespace detail {

template<typename T, typename F>
constexpr void copy(T& to, F from, unsigned int) { to = from; }

template<typename T, typename PF>
void copy(T& to, const std::unique_ptr<PF[]>& from, unsigned int) { to = from.get(); }

template<typename PT>
void copy(std::unique_ptr<PT[]>& to, const uint8_t* from, unsigned int size)
{
	if(!from)
	{
		to = {};
		return;
	}

	to = std::make_unique<PT[]>(size);
	std::memcpy(to.get(), from, size);
}

template<typename PT, typename PF>
void copy(std::unique_ptr<PT[]>& to, const std::unique_ptr<PF[]>& from, unsigned int size)
{
	if(!from)
	{
		to = {};
		return;
	}

	to = std::make_unique<PT[]>(size);
	std::memcpy(to.get(), from.get(), size);
}

} // namespace detail

template<typename P> class BasicImage;

template<typename P, typename = std::enable_if_t<std::is_convertible<P, const uint8_t*>::value>>
constexpr const uint8_t* data(const BasicImage<P>& img);

template<typename P, typename = std::enable_if_t<std::is_convertible<P, uint8_t*>::value>>
constexpr uint8_t* data(const BasicImage<P>& img);

template<typename P, typename = std::enable_if_t<std::is_const<P>::value>>
constexpr const uint8_t* data(const BasicImage<std::unique_ptr<P>>& img);

template<typename P, typename = std::enable_if_t<!std::is_const<P>::value>>
constexpr uint8_t* data(const BasicImage<std::unique_ptr<P>>& img);

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
	P data {}; // raw image data. References at least std::ceil(stride * size.y / 8.0) * 8 bytes
	nytl::Vec2ui size {}; // image size in pixels
	ImageFormat format {}; // data format in word order (endian-native)
	unsigned int stride {}; // stride in bits. At least size.x * bitSize(format)

public:
	constexpr BasicImage() = default;
	~BasicImage() = default;

	constexpr BasicImage(P xdata, nytl::Vec2ui xsize, const ImageFormat& fmt, unsigned int strd = 0)
		: data(std::move(xdata)), size(xsize), format(fmt), stride(strd)
		{ if(!stride) stride = size.x * bitSize(format); }

	template<typename O>
	constexpr BasicImage(const BasicImage<O>& lhs)
		: size(lhs.size), format(lhs.format), stride(bitStride(lhs))
		{ detail::copy(data, lhs.data, dataSize(lhs)); }

	template<typename O>
	constexpr BasicImage<P>& operator=(const BasicImage<O>& lhs)
	{
		size = lhs.size;
		format = lhs.format;
		stride = bitStride(lhs);
		detail::copy(data, lhs.data, dataSize(lhs));
		return *this;
	}

	constexpr BasicImage(const BasicImage& lhs)
		: size(lhs.size), format(lhs.format), stride(bitStride(lhs))
		{ detail::copy(data, lhs.data, dataSize(lhs)); }

	constexpr BasicImage& operator=(const BasicImage& lhs)
	{
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

/// Returns the raw data from the given BasicImage as uint8_t pointer.
/// Useful for generic code since it allows to access the raw data independently from the medium
/// used to store it (could e.g. be a std::unique_ptr).
template<typename P, typename>
constexpr uint8_t* data(const BasicImage<P>& img)
	{ return img.data; }

template<typename P, typename>
constexpr const uint8_t* data(const BasicImage<P>& img)
	{ return img.data; }

template<typename P, typename>
constexpr uint8_t* data(const BasicImage<std::unique_ptr<P>>& img)
	{ return img.data.get(); }

template<typename P, typename>
constexpr const uint8_t* data(const BasicImage<std::unique_ptr<P>>& img)
	{ return img.data.get(); }

/// Returns the stride of the given BasicImage in bits.
/// If the given image has no stride stored, calculates the stride.
template<typename P>
constexpr unsigned int bitStride(const BasicImage<P>& img)
	{ return img.stride ? img.stride : img.size.x * bitSize(img.format); }

/// Returns the stride of the given BasicImage in bytes (rounded up).
/// If the given image has no stride stored, calculates the stride.
template<typename P>
constexpr unsigned int byteStride(const BasicImage<P>& img)
	{ return img.stride ? std::ceil(img.stride / 8) : img.size.x * byteSize(img.format); }

/// Returns the total amount of bytes the image data holds (rounded up).
template<typename P>
constexpr unsigned int dataSize(const BasicImage<P>& img)
	{ return std::ceil(bitStride(img) * img.size.y / 8.0); }

/// Returns the bit of the given Image at which the pixel for the given position begins.
unsigned int pixelBit(const Image&, nytl::Vec2ui position);

/// Returns the color of the image at at the given position.
/// Does not perform any range checking, i.e. if position lies outside of the size
/// of the passed ImageData object, this call will result in undefined behavior.
nytl::Vec4u64 readPixel(const Image&, nytl::Vec2ui position);

/// Returns the color of the given pixel value for the given ImageFormat.
/// Results in undefined behaviour if the given format is invalid or the data referenced
/// by pixel does not have enough bytes to read.
/// \param bitOffset The bit position at which reading should start in signifance.
/// E.g. if bitOffset is 5, we should start reading the 5th most significant bit,
/// and then getting more significant by continuing with the 6th.
nytl::Vec4u64 readPixel(const uint8_t& pixel, const ImageFormat&, unsigned int bitOffset = 0u);

/// Sets the color of the pixel at the given position.
/// Does not perform any range checking, i.e. if position lies outside of the size
/// of the passed ImageData object, this call will result in undefined behavior.
void writePixel(const MutableImage&, nytl::Vec2ui position, nytl::Vec4u64 color);

/// Sets the color of the given pixel.
/// Results in undefined behaviour if the given format is invalid or the data referenced
/// by pixel does not have enough bytes to read.
/// \param bitOffset The bit position at which reading should start in significance.
void writePixel(uint8_t& pixel, const ImageFormat&, nytl::Vec4u64 color,
	unsigned int bitOffset = 0u);

/// Normalizes the given color values for the given format (color channel sizes).
/// Example: norm({255, 128, 511, 0}, rgba8888) returns {1.0, 0.5, 2.0, 0.0}
nytl::Vec4f norm(nytl::Vec4u64 color, const ImageFormat& format);

/// Makes sure that all color values can be represented by the number of bits their
/// channel has in the given format while keeping the color as original as possible.
nytl::Vec4u64 downscale(nytl::Vec4u64 color, const ImageFormat& format);

/// Returns whether an ImageData object satisfied the given requirements.
/// Returns false if the stride of the given ImageData satisfies the given align but
/// is not as small as possible.
/// \param strideAlign The required alignment of the stride in bits
bool satisfiesRequirements(const Image&, const ImageFormat&, unsigned int strideAlign = 0);

/// Can be used to convert image data to another format or to change its stride alignment.
/// \param alignNewStride Can be used to pass a alignment requirement for the stride of the
/// new (converted) data. Defaulted to 0, in which case the packed size will be used as stride.
/// \sa BasicImageData
/// \sa ImageDataFormat
UniqueImage convertFormat(const Image&, ImageFormat to, unsigned int alignNewStride = 0);
void convertFormat(const Image&, ImageFormat to, uint8_t& into, unsigned int alignNewStride = 0);

/// Premutliplies the alpha factors for the given image.
/// Does nothing if the given image has no alpha channel.
void premultiply(const MutableImage& img);

} // namespace nytl
