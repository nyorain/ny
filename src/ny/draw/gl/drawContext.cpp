#include <ny/draw/gl/drawContext.hpp>
#include <ny/draw/gl/texture.hpp>
#include <ny/draw/gl/context.hpp>
#include <ny/draw/gl/shader.hpp>
#include <ny/draw/freeType.hpp>
#include <ny/draw/triangulate.hpp>
#include <ny/draw/font.hpp>

#include <ny/draw/gl/validate.hpp>
#include <ny/draw/gl/shaderSources/default.hpp>
#include <ny/draw/gl/glad/glad.h>

#include <nytl/log.hpp>


namespace ny
{

vec2f GlDrawContext::asGlInvert(const vec2f& point, float ySize)
{
	return {point.x, ySize - point.y};
}

rect2f GlDrawContext::asGlInvert(const rect2f& rct, float ySize)
{
	rect2f ret = rct;
	ret.position = rct.bottomLeft();
	ret.position.y = ySize - ret.position.y;
	return ret;
}

vec2f GlDrawContext::asGlNormalize(const vec2f& point, const vec2f& size)
{
	return point / size;
}

rect2f GlDrawContext::asGlNormalize(const rect2f& rct, const vec2f& size)
{
	return {rct.position / size, rct.size / size};
}

vec2f GlDrawContext::asGlCoords(const vec2f& point, const vec2f& size)
{
	return asGlInvert(asGlNormalize(point, size));
}	

rect2f GlDrawContext::asGlCoords(const rect2f& rct, const vec2f& size)
{
	return asGlInvert(asGlNormalize(rct, size));
}

GlDrawContext::ShaderPrograms& GlDrawContext::shaderPrograms()
{
	static std::map<GlContext*, ShaderPrograms> shaderPrograms_;

	auto& prog = shaderPrograms_[GlContext::current()]; 
	if(!prog.initialized)
	{
		prog.brush.color.loadFromString(defaultShaderVS, modernColorShaderFS);
		prog.brush.colorTextureA.loadFromString(uvShaderVS, modernColorShaderTextureAFS);
		prog.initialized = 1;
	}

	return prog;
}

Shader& GlDrawContext::shaderProgramForBrush(const Brush& b)
{
	Shader* ret = nullptr;
	switch(b.type())
	{
		case Brush::Type::color:
		{
			ret = &shaderPrograms().brush.color;
			ret->use();
			ret->uniform("fColor", b.color().rgbaNorm());
			break;
		}	
		case Brush::Type::texture: 
		{
		   ret = &shaderPrograms().brush.textureRGBA;
		   ret->use();
		   ret->uniform("fTexturePosition", b.textureBrush().extents.position);
		   ret->uniform("fTextureSize", b.textureBrush().extents.size);

		   auto* glTex = dynamic_cast<const GlTexture*>(b.textureBrush().texture);
		   if(!glTex || !glTex->glTexture())
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
			ret = &shaderPrograms().brush.linearGradient;
			break;
		}
		case Brush::Type::radialGradient: 
		{
			ret = &shaderPrograms().brush.radialGradient;
			break;
		}
	}

	return *ret;
}

Shader& GlDrawContext::shaderProgramForPen(const Pen& p)
{
	Shader* ret = nullptr;
	const Brush& b = p.brush();
	switch(b.type())
	{
		case Brush::Type::color:
		{
			ret = &shaderPrograms().pen.color;
			ret->use();
			ret->uniform("fColor", b.color().rgbaNorm());
			break;
		}	
		case Brush::Type::texture: 
		{
		   ret = &shaderPrograms().pen.texture;
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
			ret = &shaderPrograms().pen.linearGradient;
			break;
		}
		case Brush::Type::radialGradient: 
		{
			ret = &shaderPrograms().pen.radialGradient;
			break;
		}
	}

	return *ret;
}

void GlDrawContext::fillTriangles(const std::vector<triangle2f>& triangles, 
		const Brush& brush, const mat3f& transMatrix)
{
	GLuint vbo, vao;
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);

	glBufferData(GL_ARRAY_BUFFER, triangles.size() * 6 * sizeof(GLfloat), 
		(GLfloat*) triangles.data(), GL_STATIC_DRAW);

	auto& program = shaderProgramForBrush(brush);
	program.uniform("vViewSize", viewport().size);
	program.uniform("vTransform", transMatrix);

	GLint posAttrib = glGetAttribLocation(program.glProgram(), "position");
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(posAttrib);
	
	glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void GlDrawContext::strokePath(const std::vector<vec2f>& path, const Pen& pen, 
	const mat3f& transMatrix)
{
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, path.size() * 3 * sizeof(GLfloat), 
		(GLfloat*) path.data(), GL_STATIC_DRAW);

	auto& program = shaderProgramForPen(pen);
	program.uniform("vViewSize", viewport().size);
	program.uniform("vTransform", transMatrix);

	GLint posAttrib = glGetAttribLocation(program.glProgram(), "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	
	glLineWidth(pen.width());
	glDrawArrays(GL_LINES, 0, path.size() * 2);
	glDeleteBuffers(1, &vbo);
}

void GlDrawContext::fillText(const Text& t, const Brush& b)
{
	if(!t.font()) return;

	auto& string = t.string();
	auto& shader = shaderPrograms().brush.colorTextureA;
	shader.use();
	shader.uniform("fColor", b.color().rgbaNorm());
	shader.uniform("vViewSize", viewport().size);
	shader.uniform("vTransform", t.transformMatrix());

	auto fontHandle = 
		static_cast<FreeTypeFontHandle*>(t.font()->getCache("ny::FreeTypeFontHandle"));
	if(!fontHandle)
	{
		auto h = make_unique<FreeTypeFontHandle>(*t.font());
		fontHandle = h.get();
		t.font()->storeCache("ny::FreeTypeFontHandle", std::move(h));
	}

	fontHandle->characterSize({0, static_cast<unsigned int>(t.size())});

	GLuint vbo, vao;
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindVertexArray(vao);
	
	vec2f position = {0, 0};
	vec2f size = {0, 0};

	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW);

	GLint posAttrib = glGetAttribLocation(shader.glProgram(), "vertex");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

	for(auto& c : string)
	{
		auto& ch = fontHandle->load(c);

        size = ch.image.size();
		auto w = size.x;
		auto h = size.y;

		auto pos = position;
		pos.x += ch.bearing.x;
        pos.y -= ch.bearing.y - t.size();

		auto xpos = pos.x;
		auto ypos = pos.y;
	
		GLfloat vertices[6][4] = {
       	     { xpos,     ypos + h,   0.0, 1.0 },            
       	     { xpos,     ypos,       0.0, 0.0 },
       	     { xpos + w, ypos,       1.0, 0.0 },

       	     { xpos,     ypos + h,   0.0, 1.0 },
       	     { xpos + w, ypos,       1.0, 0.0 },
       	     { xpos + w, ypos + h,   1.0, 1.0 }           
       	 };

		shader.uniform("fTextureSize", size);
		shader.uniform("fTexturePosition", pos);

		auto glTex = GlTexture(ch.image);
		glTex.bind();

	    glBufferSubData(GL_ARRAY_BUFFER, 0, 24 * sizeof(float), vertices); 
	
		glDrawArrays(GL_TRIANGLES, 0, 6);
		position.x += (ch.advance >> 6);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vbo);
}

void GlDrawContext::strokeText(const Text& t, const Pen& p)
{
}

void GlDrawContext::clear(const Brush& brush)
{
	VALIDATE_CTX();

	if(brush.type() == Brush::Type::color)
	{
		auto col = brush.color().rgbaNorm();
		glClearColor(col.x, col.y, col.z, col.w);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else
	{
		vec2f size = viewport().size;
		std::vector<triangle2f> triangles = {
			{{0.f, 0.f}, {size.x, 0.f}, {size.x, size.y}},
			{{0.f, 0.f}, {size.x, size.y}, {0.f, size.y}}
		};
	
		fillTriangles(triangles, brush);
	}
}

void GlDrawContext::paint(const Brush& alpha, const Brush& fill) 
{
	VALIDATE_CTX();
	nytl::sendWarning("glDC::paint: not implemented yet...");
}

void GlDrawContext::fillPreserve(const Brush& brush)
{
	VALIDATE_CTX();

	for(auto& pth : storedMask())
	{
		if(pth.type() == PathBase::Type::path)
		{
			for(auto& subpth : pth.path().subpaths())
			{
				fillTriangles(triangulate(subpth.bake()), brush, pth.transformMatrix());
			}
		}
		else if(pth.type() == PathBase::Type::rectangle)
		{
			auto points = pth.rectangle().asPath().subpaths()[0].bake();
			auto triangles = triangulate(points);
			fillTriangles(triangles, brush, pth.transformMatrix());
		}
		else if(pth.type() == PathBase::Type::circle)
		{
			fillTriangles(triangulate(pth.circle().asPath().subpaths()[0].bake()), 
					brush, pth.transformMatrix());
		}
		else if(pth.type() == PathBase::Type::text)
		{
			fillText(pth.text(), brush);
		}
	}
}

void GlDrawContext::strokePreserve(const Pen& pen)
{
	VALIDATE_CTX();

	for(auto& pth : storedMask())
	{

		if(pth.type() == PathBase::Type::path)
		{
			for(auto& subpth : pth.path().subpaths())
			{
				strokePath(subpth.bake(), pen, pth.transformMatrix());
			}
		}
		else if(pth.type() == PathBase::Type::rectangle)
		{
			auto points = pth.rectangle().asPath().subpaths()[0].bake();
			strokePath(points, pen, pth.transformMatrix());
		}
		else if(pth.type() == PathBase::Type::circle)
		{
			strokePath(pth.circle().asPath().subpaths()[0].bake(), 
					pen, pth.transformMatrix());
		}
		else if(pth.type() == PathBase::Type::text)
		{
			strokeText(pth.text(), pen);
		}
	}
}

void GlDrawContext::apply()
{
	VALIDATE_CTX();

	glFinish();
}

void GlDrawContext::clipRectangle(const rect2f& rct)
{
	VALIDATE_CTX();
	auto size = viewport().size;
	auto normRect = asGlNormalize(rct, size);

	glEnable(GL_SCISSOR_TEST);
	glScissor(normRect.position.x, normRect.position.y, normRect.size.x, normRect.size.y);
}

rect2f GlDrawContext::rectangleClip() const
{
	VALIDATE_CTX({});
	GLint vals[4];
	glGetIntegerv(GL_SCISSOR_BOX, vals);
	return {{float(vals[0]), float(vals[1])}, {float(vals[2]), float(vals[3])}};	
}

void GlDrawContext::resetRectangleClip()
{
	VALIDATE_CTX();
	glDisable(GL_SCISSOR_TEST);
}

void GlDrawContext::viewport(const rect2f& viewport)
{
	VALIDATE_CTX();
	glViewport(viewport.position.x, viewport.position.y, viewport.size.x, viewport.size.y);
}

rect2f GlDrawContext::viewport() const
{
	VALIDATE_CTX({});

	GLint vals[4];
	glGetIntegerv(GL_VIEWPORT, vals);
	return {{float(vals[0]), float(vals[1])}, {float(vals[2]), float(vals[3])}};	
}

}

