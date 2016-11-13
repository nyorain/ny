#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/bufferSurface.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Winapi BufferSurface implementation.
class WinapiBufferSurface : public BufferSurface
{
public:
	WinapiBufferSurface(WinapiWindowContext&);
	~WinapiBufferSurface();

	BufferGuard buffer() override;
	void apply(const BufferGuard&) noexcept override;

	WinapiWindowContext& windowContext() const { return *windowContext_; }

protected:
	WinapiWindowContext* windowContext_ {};
	bool active_ {};
	std::unique_ptr<std::uint8_t[]> data_;
	nytl::Vec2ui size_;
};

//WinapiBufferWindowContext
class WinapiBufferWindowContext : public WinapiWindowContext
{
public:
	WinapiBufferWindowContext(WinapiAppContext&, const WinapiWindowSettings& = {});
	~WinapiBufferWindowContext() = default;

	Surface surface() override;

protected:
	WinapiBufferSurface bufferSurface_;
};

}
