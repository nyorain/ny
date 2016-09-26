///This file holds all public event and data type ids with documentation or to link to it.
namespace eventType
{
	constexpr auto size = 11u;
	constexpr auto position = 12u;
	constexpr auto draw = 13u;
	constexpr auto show = 14u;
	constexpr auto close = 17u;

	constexpr auto mouseMove = 21u;
	constexpr auto mouseButton = 22u;
	constexpr auto mouseWheel = 23u;
	constexpr auto mouseCross = 24u;

	constexpr auto key = 25u;
	constexpr auto focus = 26u;

    constexpr auto dataOffer = 31u;

	namespace wayland
	{
		constexpr auto frameEvent = 1001u;
		constexpr auto configureEvent = 1002u;
	}

	namespace x11
	{
		constexpr unsigned int reparent = 1101;
	}
}


namespace dataType
{
	constexpr auto none = 0u; //meta symbolic constant, should not be manually used
	constexpr auto custom = 1u; //meta symbolic constant, should not be manually used

	constexpr auto raw = 2u; //std:vector<std::uint8_t>, raw unspecified data buffer
	constexpr auto text = 3u; //std::string encoded utf8
    constexpr auto filePaths = 4u; //std::vector<c++17 ? std::path : std::string>
	constexpr auto image = 5u; //ny::ImageData

	constexpr auto timePoint = 6u; //std::chrono::high_resolution_clock::time_point
	constexpr auto timeDuration = 7u; //std::chrono::high_resolution_clock::duration

	//raw, specified file buffers, represented as std::vector<std::uint8_t>, may be encoded
	constexpr auto bmp = 11u;
	constexpr auto png = 12u;
	constexpr auto jpeg = 13u;
	constexpr auto gif = 14u;

	constexpr auto mp3 = 21u;
	constexpr auto mp4 = 22u;
	constexpr auto webm = 23u;
}
