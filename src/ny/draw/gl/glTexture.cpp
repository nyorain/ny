#include <ny/draw/gl/glTexture.hpp>

#include <glpbinding/glp20/glp.h>
using namespace glp20;

namespace ny
{

namespace
{

GLenum asGlFormat(Image::Format format)
{
	switch(format)
	{
		case Image::Format::rgb888: return GL_RGB;
		case Image::Format::rgba8888: return GL_RGBA;
		case Image::Format::xrgb8888: return GL_RGBA;
		case Image::Format::a8: return GL_ALPHA; //GL_RED for later versions
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

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture_);

	glBindTexture(GL_TEXTURE_2D, texture_);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_REPEAT));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_REPEAT));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));

	auto formatGL = asGlFormat(format);
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(formatGL), size.x, 
		size.y, 0, formatGL, GL_UNSIGNED_BYTE, data);
}

void GlTexture::destroy()
{
	if(texture_)
	{
		glDeleteTextures(1, &texture_);
		texture_ = 0;
	}
}

void GlTexture::bind() const
{
	glBindTexture(GL_TEXTURE_2D, texture_);
}

void GlTexture::glTexture(unsigned int texture)
{
	destroy();
	texture_ = texture;
}

Image* GlTexture::image() const
{
	return nullptr;
}

}
