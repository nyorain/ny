#pragma once

#include <ny/include.hpp>
#include <ny/util/vec.hpp>

namespace ny
{

class image;

//todo: cursor themes for all applications. loaded with styles

enum class cursorType
{
    unknown = 0,

    none = 1,

    leftPtr = 2,
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

    image* image_ = nullptr;
    vec2i hotspot_;

public:
    cursor();
    cursor(cursorType t);
    cursor(image& data);

    void fromImage(image& data);
    void fromImage(image& data, vec2i hotspot);
    void fromNativeType(cursorType t);

    bool isImage() const;
    bool isNativeType() const;

    image* getImage() const;
    vec2i getImageHotspot() const;
    cursorType getNativeType() const;
};

}
