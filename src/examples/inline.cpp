//developement test for inline-only functionaility
//mainly useful while working on interfaces and therefore breaking the library source

#include <ny/ny.hpp>
#include <vector>

int main()
{
	std::vector<std::string> uris = {"testa", "file://tskjkl", u8"hsadf kljaööシハsd", "#comment"};
	auto encoded = ny::encodeUriList(uris);
	ny::log(encoded);

	ny::log("=============================");

	auto decoded = ny::decodeUriList(encoded);
	for(auto d : decoded) ny::log(d);
}
