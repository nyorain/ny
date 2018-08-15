// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/image.hpp> // ny::Image
#include <nytl/vec.hpp> // nytl::Vec

namespace ny {

/// Represents the different native cursor types.
/// Note that not all types are available on all backends.
enum class CursorType : unsigned int {
	unknown = 0,
	image = 1,
	none = 2,

	leftPtr, // default pointer cursor
	load, // load icon
	loadPtr, // load icon combined with default pointer
	rightPtr, // default pointer to the right (mirrored)
	hand, // a hande signaling that something can be grabbed
	grab, // some kind of grabbed cursor (e.g. closed hand)
	crosshair, // crosshair, e.g. used for move operations
	help, // help cursor sth like a question mark
	beam, // beam/caret e.g. for textfield
	forbidden, // no/not allowed, e.g. for unclickable button
	size, // general size pointer
	sizeLeft,
	sizeRight,
	sizeTop,
	sizeBottom,
	sizeBottomRight,
	sizeBottomLeft,
	sizeTopRight,
	sizeTopLeft,
};

/// Returns the associated WindowEdge for a cursorType. If the cursor type is not
/// an edge sizing cursor, returns WindowEdge::none;
/// Example: edgeFromSizeCursor(CursorType::sizeLeft) returns WindowEdge::left
WindowEdge edgeFromSizeCursor(CursorType);

/// Returns the edge sizing cursor type for the given WindowEdge. If the given WindowEdge
/// is invalid or none, returns CursorType::none.
/// Example: sizeCursorFromEdge(WindowEdge::topRight) return CursorType::sizeTopRight
CursorType sizeCursorFromEdge(WindowEdge);

/// Returns the name of a CursorType enum value. Returns "" if the cursorType is invalid.
/// Example: name(CursorType::loadPtr) return "loadPtr".
const char* name(CursorType);

/// The Cursor class represents either a native cursor image or a custom loaded image.
/// \warning The cursor does never own any image data so always check that images remain
/// valid until the cursor object is not further used. Functions taking a Cursor
/// should copy its data though.
class Cursor {
public:
	using Type = CursorType;

public:
	/// Default-constructs the Cursor with the leftPtr native type.
	constexpr Cursor() noexcept = default;

	/// Constructs the Cursor with a native cursor type.
	constexpr Cursor(CursorType type) noexcept : type_(type) {}

	/// Constructs the Cursor from an image.
	/// Note that the given ImageData must therefore remain valid while the cursor object
	/// is used.
	constexpr Cursor(const Image& img, nytl::Vec2i hotspot = {0, 0}) noexcept
		: type_(CursorType::image), image_(img), hotspot_(hotspot) {}

	/// Sets the cursor to image type and stores the given image.
	/// Note that the given ImageData must therefore remain valid while the cursor object
	/// is used.
	constexpr void image(const Image& img, nytl::Vec2i hotspot = {0, 0}) noexcept {
		type_ = CursorType::image;
		image_ = img;
		hotspot_ = hotspot;
	}

	/// Sets to cursor to the given native type.
	constexpr void nativeType(CursorType type) noexcept { type_ = type; }

	/// Returns the image of this image cursor.
	/// If the type of this cursor is not image, it will return an empty (i.e. data = nullptr)
	/// Image object.
	constexpr Image image() const noexcept {
		return (type_ == Type::image) ? image_ : Image{};
	}

	/// Returns the image hotspot.
	/// The result will be undefined when the type of this cursor is not image.
	constexpr nytl::Vec2i imageHotspot() const noexcept { return hotspot_; }

	/// Returns the type of this cursor.
	constexpr CursorType type() const noexcept { return type_; }

protected:
	CursorType type_ {CursorType::leftPtr};
	Image image_ {};
	nytl::Vec2i hotspot_ {};
};

} // namespace ny
