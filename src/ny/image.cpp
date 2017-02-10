// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/image.hpp>

#include <bitset> // std::bitset
#include <algorithm> // std::reverse
#include <cmath>

namespace ny {

bool littleEndian()
{
	constexpr uint32_t dummy = 1u;
	return ((reinterpret_cast<const uint8_t*>(&dummy))[0] == 1);
}

unsigned int bitSize(const ImageFormat& format)
{
	auto ret = 0u;
	for(auto& channel : format) ret += channel.second;
	return ret;
}

unsigned int byteSize(const ImageFormat& format)
{
	return std::ceil(bitSize(format) / 8.0);
}

ImageFormat toggleByteWordOrder(const ImageFormat& format)
{
	if(!littleEndian()) return format;

	auto copy = format;
	auto begin = copy.begin();
	auto end = copy.end();

	// ignore "empty" channels (channels with size of 0)
	// otherwise toggles formats would maybe begin with x empty channels
	// which would be valid but really ugly
	while((begin != end) && (!begin->second)) ++begin;
	while((end != (begin + 1) && (!(end - 1)->second))) --end;

	std::reverse(begin, end);
	return copy;
}

unsigned int pixelBit(const Image& image, nytl::Vec2ui pos)
{
	return image.stride * pos[1] + bitSize(image.format) * pos[0];
}

nytl::Vec4u64 readPixel(const uint8_t& pixel, const ImageFormat& format, unsigned int bitOffset)
{
	const uint8_t* iter = &pixel;
	nytl::Vec4u64 rgba {};

	for(auto i = 0u; i < format.size(); ++i) {
		// for little endian channel order is inversed
		auto channel = (littleEndian()) ? format[format.size() - (i + 1)] : format[i];
		if(!channel.second) continue;

		uint64_t* val {};
		switch(channel.first) {
			case ColorChannel::red: val = &rgba[0]; break;
			case ColorChannel::green: val = &rgba[1]; break;
			case ColorChannel::blue: val = &rgba[2]; break;
			case ColorChannel::alpha: val = &rgba[3]; break;
			case ColorChannel::none:
				iter += (bitOffset + channel.second) / 8;
				bitOffset = (bitOffset + channel.second) % 8;
				continue;
		}

		// the bitset should store least significant bits/bytes (with data) first
		std::bitset<64> bitset {};
		*val = {};

		// we simply iterate over all bytes/bits and copy them into the bitset
		// we have to respect byte endianess here
		if(littleEndian()) {
			// for little endian we can simply copy the bits into the bitset bit by bit
			// the first bytes are the least significant ones, exactly as in the bitset
			for(auto i = 0u; i < channel.second; ++i) {
				// note that this does NOT extract the bit as position bitOffset but rather
				// the bitOffset-significant bit, i.e. we don't have to care about
				// bit-endianess in any way. We want less significant bits first and this is
				// what we get here (i.e. bitOffset will always only grow, therefore the
				// extracted bits will get more significant)
				bitset[i] = (*iter & (1 << bitOffset));

				++bitOffset;
				if(bitOffset >= 8) {
					++iter;
					bitOffset = 0;
				}
			}
		} else {
			// for big endian we have to swap the order in which we read bytes
			// we start at the most significant byte we have data for (since 0xFF should
			// result in 0xFF and not 0x000...00FF) and from there go backwards, i.e.
			// less significant byte-wise.
			// Bit-wise we still get more significant during each byte inside the loop.
			// The extra check (i == chanell.second) for the next byte is needed because
			// we only want to write the first (8 - bitOffset (from beginnig)) bits of
			// the first byte.
			//
			// Example for channel.second=14, bitOffset=3.
			// the resulting bitset and the iteration i that set the bitset value:
			// <6 7 8 9 10 11 12 13 | 0 1 2 3 4 5 - - | (here are 48 untouched bits)>
			// note how i=5 is the most significant bit for the color channel here.

			auto bit = channel.second - (channel.second % 8);
			for(auto i = 0; i < channel.second; ++i) {
				// const auto bit = channel.second - (currentByte * 8) + (8 - bitOffset);
				bitset[bit] = (*iter & (1 << bitOffset));

				++bitOffset;
				++bit;
				if(bitOffset >= 8 || i == (channel.second % 8) - 1) {
					++iter;
					bit -= 8;
					bitOffset = 0;
				}
			}
		}

		// to_ullong returns an unsigned long long that has the first bits from the
		// bitset as least significant bits and the last bits from the bitset as most
		// significant bits
		*val = bitset.to_ullong();
	}

	return rgba;
}

void writePixel(uint8_t& pixel, const ImageFormat& format, nytl::Vec4u64 color,
	unsigned int bitOffset)
{
	uint8_t* iter = &pixel;

	for(auto i = 0u; i < format.size(); ++i) {
		// for little endian channel order is inversed
		auto channel = (littleEndian()) ? format[format.size() - (i + 1)] : format[i];
		if(!channel.second) continue;
		std::bitset<64> bitset;

		switch(channel.first) {
			case ColorChannel::red: bitset = color[0]; break;
			case ColorChannel::green: bitset = color[1]; break;
			case ColorChannel::blue: bitset = color[2]; break;
			case ColorChannel::alpha: bitset = color[3]; break;
			case ColorChannel::none:
				iter += (bitOffset + channel.second) / 8;
				bitOffset = (bitOffset + channel.second) % 8;
				continue;
		}

		// this is exactly like readPixel but in the opposite direction
		if(littleEndian()) {
			for(auto i = 0u; i < channel.second; ++i)
			{
				if(bitset[i]) *iter |= (1 << bitOffset);
				else *iter &= ~(1 << bitOffset);

				++bitOffset;
				if(bitOffset >= 8u) {
					bitOffset = 0u;
					++iter;
				}
			}
		} else {
			auto bit = channel.second - (channel.second % 8);
			for(auto i = 0; i < channel.second; i++) {
				if(bitset[channel.second - i]) *iter |= (1 << bitOffset);
				else *iter &= ~(1 << bitOffset);

				++bitOffset;
				++bit;
				if(bitOffset >= 8u || i == (channel.second % 8) - 1) {
					bitOffset = 0u;
					bit -= 8;
					++iter;
				}
			}
		}
	}
}

nytl::Vec4u64 readPixel(const Image& img, nytl::Vec2ui pos)
{
	auto bit = pixelBit(img, pos);
	return readPixel(*(img.data + bit / 8), img.format, bit % 8);
}

void writePixel(const MutableImage& img, nytl::Vec2ui pos, nytl::Vec4u64 color)
{
	auto bit = pixelBit(img, pos);
	return writePixel(*(img.data + bit / 8), img.format, color, bit % 8);
}

nytl::Vec4f norm(nytl::Vec4u64 color, const ImageFormat& format)
{
	auto ret = static_cast<nytl::Vec4f>(color);

	for(auto& channel : format) {
		if(!channel.second) continue;

		switch(channel.first) {
			case ColorChannel::red: ret[0] /= std::exp2(channel.second) - 1; break;
			case ColorChannel::green: ret[1] /= std::exp2(channel.second) - 1; break;
			case ColorChannel::blue: ret[2] /= std::exp2(channel.second) - 1; break;
			case ColorChannel::alpha: ret[3] /= std::exp2(channel.second) - 1; break;
			case ColorChannel::none: continue;
		}
	}

	return ret;
}

nytl::Vec4u64 downscale(nytl::Vec4u64 color, const ImageFormat& format)
{
	// find the smallest factor, i.e. the one we have to divide with
	auto factor = 1.0;
	for(auto channel : format) {
		auto value = 0u;
		switch(channel.first) {
			case ColorChannel::red: value = color[0]; break;
			case ColorChannel::green: value = color[1]; break;
			case ColorChannel::blue: value = color[2]; break;
			case ColorChannel::alpha: value = color[3]; break;
			case ColorChannel::none: continue;
		}

		if(!value) continue;
		auto highest = std::exp2(channel.second) - 1;
		factor = std::min(highest / value, factor);
	}

	return static_cast<nytl::Vec4u64>(factor * color);
}

bool satisfiesRequirements(const Image& img, const ImageFormat& format,
	unsigned int strideAlign)
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
			auto color = downscale(readPixel(img, {x, y}), to);
			auto bit = y * newStride + x * bitSize(to);
			writePixel(*(&into + bit / 8), to, color, bit % 8);
		}
	}
}

void premultiply(const MutableImage& img)
{
	auto alpha = false;
	for(auto& channel : img.format) if(channel.first == ColorChannel::alpha) alpha = true;
	if(!alpha) return;

	for(auto y = 0u; y < img.size[1]; ++y) {
		for(auto x = 0u; x < img.size[0]; ++x) {
			auto color = readPixel(img, {x, y});
			auto alpha = norm(color, img.format).w;
			color[0] *= alpha;
			color[1] *= alpha;
			color[2] *= alpha;
			writePixel(img, {x, y}, color);
		}
	}
}

} // namespace ny
