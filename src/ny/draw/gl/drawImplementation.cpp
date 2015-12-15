#include <ny/draw/gl/drawImplementation.hpp>
#include <ny/draw/gl/glContext.hpp>
#include <ny/draw/gl/glTexture.hpp>

#include <nytl/log.hpp>

#include <glpbinding/glp20/glp.h>
using namespace glp20;

namespace ny
{

vec2f GlDrawContext::Impl::asGlInvert(const vec2f& point, float ySize)
{
	return {point.x, ySize - point.y};
}

rect2f GlDrawContext::Impl::asGlInvert(const rect2f& rct, float ySize)
{
	rect2f ret = rct;
	ret.position = rct.bottomLeft();
	ret.position.y = ySize - ret.position.y;
	return ret;
}

vec2f GlDrawContext::Impl::asGlNormalize(const vec2f& point, const vec2f& size)
{
	return point / size;
}

rect2f GlDrawContext::Impl::asGlNormalize(const rect2f& rct, const vec2f& size)
{
	return {rct.position / size, rct.size / size};
}

vec2f GlDrawContext::Impl::asGlCoords(const vec2f& point, const vec2f& size)
{
	return asGlInvert(asGlNormalize(point, size));
}	

rect2f GlDrawContext::Impl::asGlCoords(const rect2f& rct, const vec2f& size)
{
	return asGlInvert(asGlNormalize(rct, size));
}

///
Shader& GlDrawContext::Impl::shaderProgramForBrush(const Brush& b)
{
	Shader* ret = nullptr;
	switch(b.type())
	{
		case Brush::Type::color:
		{
			ret = &GlDrawContext::shaderPrograms().color;
			ret->use();
			ret->uniform("fColor", b.color());
			break;
		}	
		case Brush::Type::texture: 
		{
		   ret = &GlDrawContext::shaderPrograms().texture;
		   ret->use();
		   ret->uniform("fTexturePosition", b.textureBrush().extents.position);
		   ret->uniform("fTextureSize", b.textureBrush().extents.size);

		   auto* glTex = dynamic_cast<const GlTexture*>(b.textureBrush().texture);
		   if(!glTex)
		   {
			   nytl::sendWarning("glDraw textureBrush: invalid texture");
			   break;
		   }

		   glActiveTexture(GL_TEXTURE0);
		   glTex->bind();

		   break;
		}
		case Brush::Type::linearGradient:
		{
			ret = &GlDrawContext::shaderPrograms().linearGradient;
			break;
		}
		case Brush::Type::radialGradient: 
		{
			ret = &GlDrawContext::shaderPrograms().radialGradient;
			break;
		}
	}

	return *ret;
}

bool GlDrawContext::Impl::modern()
{
	return (GlContext::current()->api() != GlContext::Api::openGL ||
			GlContext::current()->version() > 20);
}

void GlDrawContext::Impl::fillTrianglesModern(const std::vector<triangle2f>& triangles, 
		const Brush& brush, const mat3f& transMatrix)
{
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, triangles.size() * 6 * sizeof(GLfloat), 
		(GLfloat*) triangles.data(), GL_STATIC_DRAW);

	auto& program = shaderProgramForBrush(brush);
	program.uniform("vViewSize", viewport().size);
	program.uniform("vTransform", transMatrix);

	GLint posAttrib = glGetAttribLocation(program.glProgram(), "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	
	glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);
}

void GlDrawContext::Impl::fillTrianglesLegacy(const std::vector<triangle2f>& triangles, 
		const Brush& brush)
{
}

void GlDrawContext::Impl::strokePathModern(const std::vector<vec2f>& path, const Pen& brush)
{
}

void GlDrawContext::Impl::strokePathLegacy(const std::vector<vec2f>& path, const Pen& brush)
{
}

void GlDrawContext::Impl::fillTextModern(const Text& t, const Brush& b)
{
}

void GlDrawContext::Impl::fillTextLegacy(const Text& t, const Brush& b)
{
}

void GlDrawContext::Impl::strokeTextModern(const Text& t, const Pen& b)
{
}

void GlDrawContext::Impl::strokeTextLegacy(const Text& t, const Pen& b)
{
}

void GlDrawContext::Impl::fillText(const Text& t, const Brush& b)
{
	if(modern()) fillTextModern(t, b);
	else fillTextLegacy(t, b);
}

void GlDrawContext::Impl::strokeText(const Text& t, const Pen& p)
{
	if(modern()) strokeTextModern(t, p);
	else strokeTextLegacy(t, p);
}

void GlDrawContext::Impl::fillTriangles(const std::vector<triangle2f>& triangles, 
		const Brush& brush, const mat3f& transMatrix)
{
	if(modern()) fillTrianglesModern(triangles, brush, transMatrix); 
	else fillTrianglesLegacy(triangles, brush);
}

void GlDrawContext::Impl::strokePath(const std::vector<vec2f>& path, const Pen& pen)
{
	
	if(modern()) strokePathModern(path, pen); 
	else strokePathLegacy(path, pen);
}

rect2f GlDrawContext::Impl::viewport()
{
	GLint vals[4];
	glGetIntegerv(GL_VIEWPORT, vals);
	return {{float(vals[0]), float(vals[1])}, {float(vals[2]), float(vals[3])}};	
}

}

