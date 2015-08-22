#pragma once

#include <ny/include.hpp>
#include <ny/image.hpp>
#include <nyutil/vec.hpp>

namespace ny
{

class image;

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

class cursor
{
protected:
    cursorType type_ = cursorType::leftPtr;

    image image_{};
    vec2i hotspot_{};

public:
    cursor() = default;
    cursor(cursorType t);
    cursor(const image& data);

    void fromImage(const image& data);
    void fromImage(const image& data, vec2i hotspot);
    void fromNativeType(cursorType t);

    bool isImage() const;
    bool isNativeType() const;

    const image* getImage() const;
    vec2i getImageHotspot() const;
    cursorType getNativeType() const;
};

}
