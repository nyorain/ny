#include <ny/cursor.hpp>
#include <ny/windowSettings.hpp>

namespace ny
{

WindowEdge edgeFromSizeCursor(CursorType cursor)
{
	switch(cursor)
	{
		case CursorType::sizeLeft: return WindowEdge::left;
		case CursorType::sizeRight: return WindowEdge::right;
		case CursorType::sizeTop: return WindowEdge::top;
		case CursorType::sizeBottom: return WindowEdge::bottom;
		case CursorType::sizeBottomRight: return WindowEdge::bottomRight;
		case CursorType::sizeBottomLeft: return WindowEdge::bottomLeft;
		case CursorType::sizeTopRight: return WindowEdge::topRight;
		case CursorType::sizeTopLeft: return WindowEdge::topLeft;
		default: return WindowEdge::unknown;
	}
}

CursorType sizeCursorFromEdge(WindowEdge edge)
{
	switch(edge)
	{
		case WindowEdge::left: return CursorType::sizeLeft;
		case WindowEdge::right: return CursorType::sizeRight;
		case WindowEdge::top: return CursorType::sizeTop;
		case WindowEdge::bottom: return CursorType::sizeBottom;
		case WindowEdge::bottomRight: return CursorType::sizeBottomRight;
		case WindowEdge::bottomLeft: return CursorType::sizeBottomLeft;
		case WindowEdge::topRight: return CursorType::sizeTopRight;
		case WindowEdge::topLeft: return CursorType::sizeTopLeft;
		default: return CursorType::unknown;
	}
}

//Cursor
Cursor::Cursor(CursorType type) : type_(type), image_{}
{
}

Cursor::Cursor(const ImageData& img, const nytl::Vec2i& hotspot)
	: type_(CursorType::image), hotspot_(hotspot), image_(img)
{
}

void Cursor::image(const ImageData& img, const Vec2i& hotspot)
{
	image_ = img;
	hotspot_ = hotspot;

	type_ = CursorType::image;
}

void Cursor::nativeType(CursorType type)
{
	type_= type;
}

const ImageData* Cursor::image() const
{
	return (type_ == CursorType::image) ? &image_ : nullptr;
}

const Vec2i& Cursor::imageHotspot() const
{
	return hotspot_;
}

CursorType Cursor::type() const
{
	return type_;
}

}
