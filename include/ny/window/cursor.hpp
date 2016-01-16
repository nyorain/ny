#pragma once

#include <ny/window/include.hpp>
#include <ny/draw/image.hpp>
#include <nytl/vec.hpp>

namespace ny
{

//todo: cursor themes for all applications. loaded with styles

enum class cursorType
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

class Cursor
{
protected:
    cursorType type_ = cursorType::leftPtr;

    Image image_{};
    vec2i hotspot_{};

public:
    Cursor() = default;
    Cursor(cursorType t);
    Cursor(const Image& data);

    void image(const Image& data);
    void image(const Image& data, vec2i hotspot);
    void nativeType(cursorType t);

    bool isImage() const;
    bool isNativeType() const;

    const Image* image() const;
    vec2i imageHotspot() const;
    cursorType nativeType() const;
};

}
