//developement test for inline-only functionaility
//mainly useful while working on interfaces and therefore breaking the library source

#include <ny/image.hpp>
#include <ny/log.hpp>
#include <vector>

using namespace image;

int main()
{
	constexpr ImageFormat rgb333 {{
		{ColorChannel::red, 3},
		{ColorChannel::green, 3},
		{ColorChannel::blue, 3},
	}};

	constexpr ImageFormat rgb666 {{
		{ColorChannel::red, 6},
		{ColorChannel::green, 6},
		{ColorChannel::blue, 6},
	}};

	constexpr ImageFormat rgb161616 {{
		{ColorChannel::red, 16},
		{ColorChannel::green, 16},
		{ColorChannel::blue, 16},
	}};

	constexpr ImageFormat rgb141414 {{
		{ColorChannel::red, 14},
		{ColorChannel::green, 14},
		{ColorChannel::blue, 14},
	}};

	ny::debug(std::hex, "All color given in {R, G, B, A}\n\n");

	/*
	{
		std::uint32_t pixel = 0xAABBCCDD;
		auto& pixeldata = reinterpret_cast<std::uint8_t&>(pixel);

		ny::debug("\n\n0xAABBCCDD");
		ny::debug("rgba8888: ", readPixel(pixeldata, rgba8888));
		ny::debug("argb8888: ", readPixel(pixeldata, argb8888));
		ny::debug("a8: ", readPixel(pixeldata, a8));
		ny::debug("a4: ", readPixel(pixeldata, a4));
		ny::debug("a1: ", readPixel(pixeldata, a1));
	}

	{
		std::uint8_t pixeldata[] = {0xAA, 0xBB, 0xCC, 0xDD};
		ny::debug("\n\n{0xAA, 0xBB, 0xCC, 0xDD}");
		ny::debug("rgba8888: ", readPixel(*pixeldata, rgba8888));
		ny::debug("argb8888: ", readPixel(*pixeldata, argb8888));
		ny::debug("a8: ", readPixel(*pixeldata, a8));
		ny::debug("a4: ", readPixel(*pixeldata, a4));
		ny::debug("a1: ", readPixel(*pixeldata, a1));
	}

	{
		std::uint8_t pixeldata[] = {0xAA, 0xBB, 0xCC, 0xDD};
		ny::debug("\n\n{0xAA, 0xBB, 0xCC, 0xDD}");
		ny::debug("toggle(rgba8888): ", readPixel(*pixeldata, toggleByteWordOrder(rgba8888)));
		ny::debug("toggle(argb8888): ", readPixel(*pixeldata, toggleByteWordOrder(argb8888)));
		ny::debug("toggle(a8): ", readPixel(*pixeldata, toggleByteWordOrder(a8)));
	}

	{
		// 0x7 0x1 0x2
		// 111 001 010
		std::uint32_t pixel = 0b111001010;
		auto& pixeldata = reinterpret_cast<std::uint8_t&>(pixel);

		ny::debug("\n\n0b_111_001_010");
		ny::debug("rgb333: ", readPixel(pixeldata, rgb333));
		ny::debug("a8: ", readPixel(pixeldata, a8));
	}

	{
		// 0x11   0xc    0x25
		// 010001 001100 100101
		std::uint32_t pixel = 0b010001001100100101;
		auto& pixeldata = reinterpret_cast<std::uint8_t&>(pixel);

		ny::debug("\n\n0b_010001_001100_100101");
		ny::debug("rgb666: ", readPixel(pixeldata, rgb666));
		ny::debug("rgb333: ", readPixel(pixeldata, rgb333));
		ny::debug("toggle(rgb333): ", readPixel(pixeldata, toggleByteWordOrder(rgb333)));
	}

	{
		std::uint64_t pixel = 0xFFFFFFFFCCCC;
		auto& pixeldata = reinterpret_cast<std::uint8_t&>(pixel);

		ny::debug("\n\n0xAAAABBBBCCCC");
		ny::debug("rgb161616: ", readPixel(pixeldata, rgb161616));
		ny::debug("rgb141414: ", readPixel(pixeldata, rgb141414));
		ny::debug("norm(rgb161616): ", norm(readPixel(pixeldata, rgb161616), rgb161616));
		ny::debug("norm(rgb141414): ", norm(readPixel(pixeldata, rgb141414, 6), rgb141414));
	}

	{
		nytl::Vec4u64 color = {0xAA, 0xBB, 0xCC, 0xDD};
		std::uint8_t pixeldata[4] {};

		auto debug = [&](const char* format){
			std::uint32_t pixel;
			std::memcpy(&pixel, pixeldata, 4);
			ny::debug(format, pixel);
		};

		ny::debug("\n\n0xAABBCCDD");
		writePixel(*pixeldata, rgba8888, color); debug("rgba8888: "); std::memset(pixeldata, 0, 4);
		writePixel(*pixeldata, argb8888, color); debug("argb8888: "); std::memset(pixeldata, 0, 4);
		writePixel(*pixeldata, a8, color); debug("a8: "); std::memset(pixeldata, 0, 4);
		writePixel(*pixeldata, a1, color); debug("a1: "); std::memset(pixeldata, 0, 4);
	}
	*/

	std::cout << std::hex;
	auto dump = [](auto&& img) {
		for(auto y = 0u; y < img.size[0]; ++y)
			for(auto x = 0u; x < img.size[0]; ++x)
				std::cout << readPixel(img, {x, y}) << "\n";

		std::cout << "=====\n\n";
	};

	constexpr std::uint32_t imageData[] = {
		0x11223344,
		0xAABBCCDD,
		0x0000FF00,
		0xFFFFEEFF
	};

	// auto bytes = reinterpret_cast<const std::uint8_t*>(imageData);
	std::uint8_t bytes[4 * 4];
	std::memcpy(bytes, imageData, 4 * 4);
	Image image(bytes, {2, 2}, rgba8888);
	dump(image);

	auto image2 = convertFormat(image, argb8888);
	// OwnedImage image2 = image;
	dump(image2);
	// dump(image2);

	premultiply(image2);
	dump(image2);
	// dump(image2);

	return 1;
}
