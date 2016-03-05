#include <ny/draw/gl/texture.hpp>
#include <ny/draw/gl/context.hpp>
#include <ny/draw/gl/glad/glad.h>

namespace ny
{

namespace
{

unsigned int asGlFormat(Image::Format format)
{
	switch(format)
	{
		case Image::Format::rgb888: return static_cast<unsigned int>(GL_RGB);
		case Image::Format::rgba8888: return static_cast<unsigned int>(GL_RGBA);
		case Image::Format::xrgb8888: return static_cast<unsigned int>(GL_RGBA);
		case Image::Format::a8: return static_cast<unsigned int>(GL_RED); //GL_RED later
	}
}

}

//
GlTexture::GlTexture(const Image& img)
{
	create(img);
}

GlTexture::~GlTexture()
{
	destroy();
}

void GlTexture::create(const Image& content)
{
	create(content.size(), content.data(), content.format());
}

void GlTexture::create(const Vec2ui& size, const std::uint8_t* data, Image::Format format)
{
	destroy();

	size_ = size;
	format_ = format;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture_);

	glBindTexture(GL_TEXTURE_2D, texture_);

//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_REPEAT));
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_REPEAT));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));


	auto formatGL = asGlFormat(format);
	glTexImage2D(GL_TEXTURE_2D, 0, formatGL, size.x, 
		size.y, 0, formatGL, GL_UNSIGNED_BYTE, data);
}

void GlTexture::destroy()
{
	if(texture_)
	{
		glDeleteTextures(1, &texture_);
		texture_ = 0;
	}

	size_ = {0u, 0u};
}

void GlTexture::bind(unsigned int place) const
{
	glActiveTexture(GL_TEXTURE0 + place);
	glBindTexture(GL_TEXTURE_2D, texture_);
}

void GlTexture::glTexture(unsigned int texture, const Vec2ui& siz, Image::Format format)
{
	destroy();
	texture_ = texture;
	format_ = format;

	size(siz);
}

Image* GlTexture::image() const
{
	if(!GlContext::current() || !texture_) return nullptr;
	bind();

	if(GlContext::current()->api() == GlContext::Api::openGL)
	{
		auto psize = Image::formatSize(format_);
		auto pixels = std::make_unique<std::uint8_t[]>(size_.x * size_.y * psize);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

		return new Image(std::move(pixels), size_, Image::Format::rgba8888);
	}
	else if(GlContext::current()->api() == GlContext::Api::openGLES)
	{
		GLint previousFrameBuffer;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFrameBuffer);

		unsigned int fb;
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);

		auto psize = Image::formatSize(format_);
		auto pixels = std::make_unique<std::uint8_t[]>(size_.x * size_.y * psize);
		glReadPixels(0, 0, size_.x, size_.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

		glDeleteFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, previousFrameBuffer);

		return new Image(std::move(pixels), size_, Image::Format::rgba8888);
	}

	return nullptr;
}


}
