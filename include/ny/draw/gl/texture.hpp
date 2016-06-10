#pragma once

#include <ny/include.hpp>
#include <ny/draw/texture.hpp>
#include <ny/base/image.hpp>
#include <ny/draw/color.hpp>
#include <ny/draw/gl/resource.hpp>

#include <nytl/vec.hpp>

namespace ny
{

///Texture implementation for openGL(ES).
class GlTexture : public Texture, public GlResource
{
protected:
	Image::Format format_;
	unsigned int texture_ = 0;
	Vec2ui size_ {0u, 0u};

public:
	GlTexture() = default;
	GlTexture(const Image& context);
	virtual ~GlTexture();

	GlTexture(GlTexture&& other) noexcept = default;
	GlTexture& operator=(GlTexture&& other) noexcept = default;

	void create(const Image& content);
	void create(const Vec2ui& size, const std::uint8_t* data, Image::Format format);

	void destroy();

	const Vec2ui& size() const { return size_; }
	void size(const Vec2ui& size) { size_ = size; };

	unsigned int glTexture() const { return texture_; }
	void glTexture(unsigned int texture, const Vec2ui& size, Image::Format format);

	void bind(unsigned int place = 0) const;

	virtual bool shareable() const override { return 1; }
	virtual Image* image() const override;
};

}
