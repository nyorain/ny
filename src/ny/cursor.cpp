// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

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

const char* name(CursorType cursor)
{
	switch(cursor)
	{
		case CursorType::leftPtr: return "leftPtr";
		case CursorType::load: return "load";
		case CursorType::loadPtr: return "loadPtr";
		case CursorType::rightPtr: return "rightPtr";
		case CursorType::hand: return "hand";
		case CursorType::grab: return "grab";
		case CursorType::crosshair: return "crosshair";
		case CursorType::help: return "help";
		case CursorType::size: return "size";
		case CursorType::sizeLeft: return "sizeLeft";
		case CursorType::sizeRight: return "sizeRight";
		case CursorType::sizeTop: return "sizeTop";
		case CursorType::sizeBottom: return "sizeBottom";
		case CursorType::sizeBottomRight: return "sizeBottomRight";
		case CursorType::sizeBottomLeft: return "sizeBottomLeft";
		case CursorType::sizeTopRight: return "sizeTopRight";
		case CursorType::sizeTopLeft: return "sizeTopLeft";
		default: return "";
	}
}

//Cursor
Cursor::Cursor(const ImageData& img, nytl::Vec2i hotspot) noexcept
	: type_(CursorType::image), hotspot_(hotspot), image_(img)
{
}

void Cursor::image(const ImageData& img, Vec2i hotspot) noexcept
{
	image_ = img;
	hotspot_ = hotspot;

	type_ = CursorType::image;
}

}
