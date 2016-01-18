#pragma once

#include <ny/include.hpp>
#include <ny/draw/texture.hpp>
#include <ny/draw/image.hpp>
#include <ny/draw/color.hpp>

#include <nytl/vec.hpp>
#include <nytl/nonCopyable.hpp>

namespace ny
{

///Texture implementation for openGL(ES).
class GlTexture : public Texture, public nonCopyable
{
protected:
	unsigned int texture_ = 0;
	vec2ui size_ {0u, 0u};

public:
	GlTexture() = default;
	virtual ~GlTexture();

	void create(Image& content);
	void create(const vec2ui& size, const unsigned char* data, Image::Format format);

	void destroy();

	const vec2ui& size() const { return size_; }
	void size(const vec2ui& size) { size_ = size; };

	unsigned int glTexture() const { return texture_; }
	void glTexture(unsigned int texture, const vec2ui& size);

	void bind() const;

	virtual Image* image() const override;
};

}
