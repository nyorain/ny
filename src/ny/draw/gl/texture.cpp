#include <ny/draw/gl/texture.hpp>
#include <ny/draw/gl/context.hpp>

#include <glpbinding/glp20/glp.h>
#include <glesbinding/gles/gles.h>
#include <glbinding/gl/gl.h>

namespace ny
{

namespace
{

unsigned int asGlFormat(Image::Format format)
{
	using namespace glp;
	switch(format)
	{
		case Image::Format::rgb888: return static_cast<unsigned int>(GL_RGB);
		case Image::Format::rgba8888: return static_cast<unsigned int>(GL_RGBA);
		case Image::Format::xrgb8888: return static_cast<unsigned int>(GL_RGBA);
		case Image::Format::a8: return static_cast<unsigned int>(GL_ALPHA); //GL_RED later
	}
}

}

//
GlTexture::~GlTexture()
{
	destroy();
}

void GlTexture::create(Image& content)
{
	create(content.size(), content.data().data(), content.format());
}

void GlTexture::create(const vec2ui& size, const unsigned char* data, Image::Format format)
{
	destroy();
	size_ = size;

	using namespace glp20;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture_);

	glBindTexture(GL_TEXTURE_2D, texture_);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_REPEAT));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_REPEAT));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));

	auto formatGL = asGlFormat(format);
	glTexImage2D(GL_TEXTURE_2D, 0, formatGL, size.x, 
		size.y, 0, static_cast<GLenum>(formatGL), GL_UNSIGNED_BYTE, data);
}

void GlTexture::destroy()
{
	using namespace glp;
	if(texture_)
	{
		glDeleteTextures(1, &texture_);
		texture_ = 0;
	}

	size_ = {0u, 0u};
}

void GlTexture::bind() const
{
	using namespace glp;
	glBindTexture(GL_TEXTURE_2D, texture_);
}

void GlTexture::glTexture(unsigned int texture, const vec2ui& siz)
{
	destroy();
	texture_ = texture;

	size(siz);
}

Image* GlTexture::image() const
{
	if(!GlContext::current() || !texture_) return nullptr;
	bind();

	if(GlContext::current()->api() == GlContext::Api::openGL)
	{
		using namespace gl;
		/*
		vec2i size;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &size.x);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &size.y);
		*/

		std::vector<unsigned char> pixels(size_.x * size_.y * 4);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		return new Image(std::move(pixels), size_, Image::Format::rgba8888);
	}
	else if(GlContext::current()->api() == GlContext::Api::openGLES)
	{
		using namespace gles;

		GLint previousFrameBuffer;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFrameBuffer);

		unsigned int fb;
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);

		std::vector<unsigned char> pixels(size_.x * size_.y * 4);
		glReadPixels(0, 0, size_.x, size_.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		glDeleteFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, previousFrameBuffer);

		return new Image(std::move(pixels), size_, Image::Format::rgba8888);
	}

	return nullptr;
}


}
