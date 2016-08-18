///This file holds all public event and data type ids with documentation or to link to it.
namespace eventType
{
	constexpr auto windowSize = 11u;
	constexpr auto windowPosition = 12u;
	constexpr auto windowDraw = 13u;
	constexpr auto windowShow = 14u;
	constexpr auto windowFocus = 15u;
	constexpr auto windowRefresh = 16u;
	constexpr auto windowClose = 17u;

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
	constexpr auto none = 0u; //meta symbolic constant, dont actually use it.
	constexpr auto custom = 1u; //meta symbolic constant, dont actually use it.

	constexpr auto raw = 2u; //std:vector<std::uint8_t>, raw unspecified data buffer
	constexpr auto text = 3u; //std::string encoded utf8
    constexpr auto filePaths = 4u; //std::vector<c++17 ? std::path : std::string>
	constexpr auto image = 5u; //ny:Image
	constexpr auto time = 6u; //ny(tl)::TimePoint
}
