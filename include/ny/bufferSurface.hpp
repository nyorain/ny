// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/image.hpp> // ny::MutableImage
#include <nytl/nonCopyable.hpp> // nytl::NonCopyable

namespace ny {

/// Software drawable buffer manager. Abstract base class for backends to implement.
/// Can be used to retrieve a BufferGuard which can be used to access a raw pixel buffer
/// that can be accessed to change the associated Surfaces contents.
class BufferSurface {
public:
	BufferSurface() = default;
	virtual ~BufferSurface() = default;

	/// Returns a BufferGuard that can be used to draw onto this surface.
	/// The contents of the BufferGuard will be applied on its destruction.
	/// If this function is called when there is already an active BufferGuard for
	/// this surface or constructing one fails in any other way, this function should
	/// throw an exception.
	/// It is not allowed to dispatch events while a BufferGuard is alive, this
	/// means that if one if received during an event callback it must be
	/// destructed before the callback returns.
	/// \sa BufferGuard
	virtual BufferGuard	buffer() = 0;

	//TODO: some way to query (if) currently active BufferGuard?
	//TODO: some way to query size?
	//TODO: just pass an Image with the surface contents directly?
	//			could be even safer in some cases since it can be directly applied

protected:
	/// Called from the BufferGuard destructor and should apply the contents
	/// of the BufferGuard. If this function call or the BufferGuard parameter are invalid
	/// this function should not throw, but outputs a warning.
	virtual void apply(const BufferGuard&) noexcept = 0;
	friend class BufferGuard;
};

// TODO: some way to make this nonmovable?
// would generally forbid to manage its lifetime manually

/// Manages a drawable buffer in form of an ImageData object.
/// When this Guard object is destructed, it will apply the buffer it holds.
/// Should generally be used as scope guards and not with manual managed lifetime like
/// e.g. when wrapped in a smart pointer.
class BufferGuard : public nytl::NonCopyable {
public:
	BufferGuard(BufferSurface& surf, const MutableImage& img) : surface_(surf), img_(img) {}
	~BufferGuard() { surface_.apply(*this); }

	BufferGuard(BufferGuard&&) noexcept = default;
	BufferGuard& operator=(BufferGuard&&) noexcept = default;

	/// Returns a mutable image data buffer to draw into. When this guard get destructed, the
	/// buffer it holds will be applied to the associated surface.
	/// Note that the format of this image data buffer may vary on different backends and must
	/// be taken into account to achieve correct color values.
	/// Can only be called on lvalues since calling it on temporaries does not make sense.
	/// The object must not be used any longer after the BufferGuard goes out of scope.
	/// \sa BasicImageData
	/// \sa convertFormat
	/// \sa BufferSurface
	const MutableImage& get() const & { return img_; }

	/// Returns the BufferSurface associated with this BufferGuard.
	/// The BufferGuard was retrieved from the BufferSurface and will call
	/// BufferSurface::apply on destruction.
	/// \sa BufferSurface
	BufferSurface& bufferSurface() const & { return surface_; }

protected:
	BufferSurface& surface_;
	MutableImage img_;
};

} // namespace ny
