//developement test for inline-only functionaility
//mainly useful while working on interfaces and therefore breaking the library source

#include <ny/ny.hpp>
#include <vector>

namespace ny
{

nytl::Vec4u8 formatDataColor(const std::uint8_t& pixel, ny::ImageDataFormat format);

}

int main()
{
	// std::vector<std::string> uris = {"testa", "file://tskjkl", u8"hsadf kljaööシハsd", "#comment"};
	// auto encoded = ny::encodeUriList(uris);
	// ny::log(encoded);
	//
	// ny::log("=============================");
	//
	// auto decoded = ny::decodeUriList(encoded);
	// for(auto d : decoded) ny::log(d);

	// std::uint8_t pixel1[4] = {0xAA, 0xBB, 0xCC, 0xDD};
	// std::uint32_t pixel2 = 0xAABBCCDD;
	std::uint8_t pixel2 = 0xAA;

	// std::cout << std::hex << (nytl::Vec4u32) ny::formatDataColor(*pixel1, ny::ImageDataFormat::rgba8888) << "\n";
	std::cout << std::hex << (nytl::Vec4u32) ny::formatDataColor((uint8_t&)pixel2, ny::ImageDataFormat::a8) << "\n";

	return 1;
}
