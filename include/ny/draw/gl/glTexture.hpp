#pragma once

#include <ny/draw/include.hpp>
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

public:
	GlTexture() = default;
	virtual ~GlTexture();

	void create(Image& content);
	void create(const vec2ui& size, const unsigned char* data, Image::Format format);

	void destroy();

	unsigned int glTexture() const { return texture_; }
	void glTexture(unsigned int texture);

	void bind() const;

	virtual Image* image() const override;
};

}
