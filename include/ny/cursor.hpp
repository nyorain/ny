#pragma once

namespace ny
{

class image;

//todo: cursor themes for all applications. loaded with styles

enum class cursorType
{
    Unknown = 0,

    none = 1,

    LeftPtr = 2,
    RightPtr,
    Load,
	LoadPtr,
    Hand,
    Grab,
    Crosshair,
    Size,
    SizeLeft,
    SizeRight,
    SizeTop,
    SizeBottom,
    SizeBottomRight,
    SizeBottomLeft,
    SizeTopRight,
    SizeTopLeft
};

class cursor
{
protected:
    cursorType type_;
    image* image_;

public:
    cursor();
    cursor(cursorType t);
    cursor(image& data);

    void fromImage(image& data);
    void fromNativeType(cursorType t);

    bool isImage() const;
    bool isNativeType() const;

    image* getImage() const;
    cursorType getNativeType() const;
};

}
