#include <ny/base/utf.hpp>
#include <locale>
#include <codecvt>

namespace ny
{

namespace
{

template<typename CVT>
class Codecvt : public CVT
{
public:
	Codecvt() = default;
	~Codecvt() = default;
};

template<typename CVT> std::basic_string<typename CVT::extern_type>
convertOut(const std::basic_string<typename CVT::intern_type>& str)
{
	using IT = typename CVT::intern_type;
	using ET = typename CVT::extern_type;

	const IT* fromNext;
	ET* toNext;

	std::basic_string<ET> ret(str.size(), '\0');
	std::mbstate_t state;
	Codecvt<CVT> cvt;
	cvt.out(state, &*str.begin(), &*str.end(), fromNext, &*ret.begin(), &*ret.end(), toNext);

	return ret;
}

template<typename CVT> std::basic_string<typename CVT::intern_type>
convertIn(const std::basic_string<typename CVT::extern_type>& str)
{
	using IT = typename CVT::intern_type;
	using ET = typename CVT::extern_type;

	const ET* fromNext;
	IT* toNext;

	std::basic_string<IT> ret(str.size(), '\0');
	std::mbstate_t state;
	Codecvt<CVT> cvt;
	cvt.in(state, &*str.begin(), &*str.end(), fromNext, &*ret.begin(), &*ret.end(), toNext);

	return ret;
}

}

std::u16string utf8to16(const std::string& utf8)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.from_bytes(utf8);
}
std::u32string utf8to32(const std::string& utf8)
{
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
	return converter.from_bytes(utf8);
}
std::string utf16to8(const std::u16string& utf16)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.to_bytes(utf16);
}
std::u32string utf16to32(const std::u16string& utf16)
{
	return utf8to32(utf16to8(utf16));
}
std::string utf32to8(const std::u32string& utf32)
{
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
	return converter.to_bytes(utf32);
}
std::u16string utf32to16(const std::u32string& utf32)
{
	return utf8to16(utf32to8(utf32));
}

}
