// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/imageData.hpp>
#include <nytl/nonCopyable.hpp>

//This header and its functionality can be used without linking to ny.

namespace ny
{

///Software drawable buffer manager. Abstract base class for backends to implement.
///Can be used to retrieve a BufferGuard which can be used to access a raw pixel buffer
///that can be accessed to change the associated Surfaces contents.
class BufferSurface
{
public:
	BufferSurface() = default;
	virtual ~BufferSurface() = default;

	///Returns a BufferGuard that can be used to draw onto this surface.
	///The contents of the BufferGuard will be applied on its destruction.
	///If this function is called when there is already an active BufferGuard for
	///this surface or constructing one fails in any other way, this function should
	///throw an exception.
	///\sa BufferGuard
	virtual BufferGuard	buffer() = 0;

	//TODO: some way to query (if) currently active BufferGuard?

protected:
	///This function is called from the BufferGuard destructor and should apply the contents
	///of the BufferGuard. If this function call or the BufferGuard parameter are invalid
	///this function should not throw, but outputs a warning.
	virtual void apply(const BufferGuard&) noexcept = 0;
	friend class BufferGuard;
};

///Manages a drawable buffer in form of an ImageData object.
///When this Guard object is destructed, it will apply the buffer it holds.
class BufferGuard : public nytl::NonCopyable
{
public:
	BufferGuard(BufferSurface& surf, const MutableImageData& data) : surface_(surf), data_(data) {}
	~BufferGuard() { surface_.apply(*this); }

	BufferGuard(BufferGuard&&) noexcept = default;
	BufferGuard& operator=(BufferGuard&&) noexcept = default;

	///Returns a mutable image data buffer to draw into. When this guard get destructed, the
	///buffer it holds will be applied to the associated surface.
	///Note that the format of this image data buffer may vary on different backends and must
	///be taken into account to achieve correct color values.
	///\sa BasicImageData
	///\sa convertFormat
	///\sa BufferSurface
	const MutableImageData& get() const & { return data_; }

	///Returns the BufferSurface associated with this BufferGuard.
	///The BufferGuard was retrieved from the BufferSurface and will call
	///BufferSurface::apply on destruction.
	///\sa BufferSurface
	BufferSurface& bufferSurface() const & { return surface_; }

protected:
	BufferSurface& surface_;
	MutableImageData data_;
};

}
