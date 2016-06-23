#pragma once

#include <ny/include.hpp>
#include <ny/base/image.hpp>
#include <nytl/vec.hpp>

namespace ny
{

//TODO: cursor themes for all applications. loaded with styles
///The Cursor class represents either a native cursor image or a custom loaded image.
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
		no,
    	crosshair,
		help,
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

public:
	///Default-constructs the Cursor with the leftPtr native type.
    Cursor() = default;

	///Constructs the Cursor with a custom type.
    Cursor(Type t);

	///Constructs the Cursor from an image.
    Cursor(const Image& data);


	///Sets the cursor to image type and stores a copy of the given image.
	///Does not change the hotspot of the image which is by default {0, 0};
    void image(const Image& data);

	///Sets the cursor to image type and stores a copy of the given image.
	///The hotspot of the cursor(i.e. the point where the real cursor position is) is set to the
	///given hotspot.
    void image(const Image& data, const Vec2i& hotspot);

	///Sets to cursor to the given native type.
    void nativeType(Type t);

	///Returns whether the cursor is a custom image cursor.
    bool isImage() const;

	///Returns wheter the cursor is a native type.
    bool isNativeType() const;

	///Returns the image of this image cursor, or nullptr if it is a native type cursor.
    const Image* image() const;

	///Returns the image hotspot.
    Vec2i imageHotspot() const;

	///Returns the type of this cursor.
    Type type() const;

protected:
    Type type_ = Type::leftPtr;
    Image image_{};
    Vec2i hotspot_{};
};

}
