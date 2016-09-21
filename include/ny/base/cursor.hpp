#pragma once

#include <ny/include.hpp>
#include <nytl/vec.hpp>
#include <evg/image.hpp>
#include <memory>

namespace ny
{

///Represents the different native cursor types.
///Note that not all types are available on all backends.
enum class CursorType : unsigned int
{
	unknown = 0,
	image = 1,
	none = 2,

	leftPtr, //default pointer cursor
	load, //load icon
	loadPtr, //load icon combined with default pointer
	rightPtr, //default pointer to the right (mirrored)
	hand, //a hande signaling that something can be grabbed
	grab, //some kind of grabbed cursor (e.g. closed hand)
	crosshair, //crosshair, e.g. used for move operations
	help, //help cursor sth like a question mark
	size, //general size pointer
	sizeLeft,
	sizeRight,
	sizeTop,
	sizeBottom,
	sizeBottomRight,
	sizeBottomLeft,
	sizeTopRight,
	sizeTopLeft

	//additional cursor types (?)
	//fobidden/no
};


//TODO: cursor themes for all applications. loaded with styles
///The Cursor class represents either a native cursor image or a custom loaded image.
class Cursor
{
public:
	///Default-constructs the Cursor with the leftPtr native type.
    Cursor() = default;

	///Constructs the Cursor with a native cursor type.
    Cursor(CursorType type);

	///Constructs the Cursor from an image.
    Cursor(const evg::Image& img, const nytl::Vec2i& hotspot = {0, 0});
	Cursor(evg::Image&& img, const nytl::Vec2i& hotspot = {0, 0});

	///Sets the cursor to image type and stores a copy of the given image.
	///Does not change the hotspot of the image which is by default {0, 0};
    void image(const evg::Image& image, const nytl::Vec2i& hotspot = {0, 0});
    void image(evg::Image&& image, const nytl::Vec2i& hotspot = {0, 0});

	///Sets to cursor to the given native type.
    void nativeType(CursorType type);

	///Returns the image of this image cursor, or nullptr if it is a native cursor type.
    evg::Image* image();
    const evg::Image* image() const;

	///Returns the image hotspot.
	///The result will be undefined when the type of this cursor is not image.
    Vec2i imageHotspot() const;

	///Returns the type of this cursor.
    CursorType type() const;

protected:
    CursorType type_ = CursorType::leftPtr;
	nytl::Vec2i hotspot_{};
	ImageData data_{};
};

}
