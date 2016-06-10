#pragma once

#include <ny/include.hpp>
#include <ny/draw/image.hpp>
#include <nytl/vec.hpp>

namespace ny
{

//todo: cursor themes for all applications. loaded with styles
class Cursor
{
public:
	enum class Type
	{
		unknown = 0,
    	image = 1,
    	none = 2,

    	leftPtr,
    	rightPtr,
    	load,
		loadPtr,
    	hand,
    	grab,
    	crosshair,
    	size,
    	sizeLeft,
    	sizeRight,
    	sizeTop,
    	sizeBottom,
    	sizeBottomRight,
    	sizeBottomLeft,
    	sizeTopRight,
    	sizeTopLeft
	};

protected:
    Type type_ = Type::leftPtr;

    Image image_{};
    Vec2i hotspot_{};

public:
    Cursor() = default;
    Cursor(Type t);
    Cursor(const Image& data);

    void image(const Image& data);
    void image(const Image& data, const Vec2i& hotspot);
    void nativeType(Type t);

    bool isImage() const;
    bool isNativeType() const;

    const Image* image() const;
    Vec2i imageHotspot() const;
    Type type() const;
};

}
