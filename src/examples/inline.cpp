//developement test for inline-only functionaility
//mainly useful while working on interfaces and therefore breaking the library source

#include <ny/image.hpp>
#include <ny/log.hpp>
#include <vector>

int main()
{
	// std::uint32_t pixel = 0xAABBCCDD;
	std::uint32_t pixel = 0x1;

	auto& pixeldata = reinterpret_cast<uint8_t&>(pixel);
	ny::debug(std::hex, "color: ", image::readPixel(pixeldata, image::r1));
	return 1;
}
