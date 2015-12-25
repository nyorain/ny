#include <ny/draw/gl/glDrawContext.hpp>
#include <ny/draw/gl/glTexture.hpp>
#include <ny/draw/gl/glContext.hpp>
#include <ny/draw/gl/shader.hpp>
#include <ny/draw/triangulate.hpp>

#include <ny/draw/gl/drawImplementation.hpp>
#include <ny/draw/gl/shaderSources/modernSources.hpp>

#include <nytl/log.hpp>

#include <glpbinding/glp20/glp.h>
using namespace glp20;

//macro for current context validation
#if defined(__GNUC__) || defined(__clang__)
 #define FUNC_NAME __PRETTY_FUNCTION__
#else
 #define FUNC_NAME __func__
#endif //FUNCNAME

//macro for assuring a valid context (warn and return if there is none)
#define VALIDATE_CTX(...) if(!GlContext::current())\
	{ nytl::sendWarning(FUNC_NAME, ": no current opengl context."); return __VA_ARGS__; }


namespace ny
{

GlDrawContext::ShaderPrograms& GlDrawContext::shaderPrograms()
{
	//todo: context version, api
	
	auto& prog = shaderPrograms_[GlContext::current()]; 
	if(!prog.initialized)
	{
		prog.brush.color.loadFromString(defaultShaderVS, modernColorShaderFS);
		prog.brush.texture.loadFromString(defaultShaderVS, modernTextureShaderFS);
		prog.initialized = 1;
	}

	return prog;
}

////
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
	
		Impl::fillTriangles(triangles, brush, ny::identityMat<3>());
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
				Impl::fillTriangles(triangulate(subpth.bake()), brush, pth.transformMatrix());
			}
		}
		else if(pth.type() == PathBase::Type::rectangle)
		{
			auto points = pth.rectangle().asPath().subpaths()[0].bake();
			auto triangles = triangulate(points);
			Impl::fillTriangles(triangles, brush, pth.transformMatrix());
		}
		else if(pth.type() == PathBase::Type::circle)
		{
			Impl::fillTriangles(triangulate(pth.circle().asPath().subpaths()[0].bake()), 
					brush, pth.transformMatrix());
		}
		else if(pth.type() == PathBase::Type::text)
		{
			Impl::fillText(pth.text(), brush);
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
				Impl::strokePath(subpth.bake(), pen, pth.transformMatrix());
			}
		}
		else if(pth.type() == PathBase::Type::rectangle)
		{
			auto points = pth.rectangle().asPath().subpaths()[0].bake();
			Impl::strokePath(points, pen, pth.transformMatrix());
		}
		else if(pth.type() == PathBase::Type::circle)
		{
			Impl::strokePath(pth.circle().asPath().subpaths()[0].bake(), 
					pen, pth.transformMatrix());
		}
		else if(pth.type() == PathBase::Type::text)
		{
			Impl::strokeText(pth.text(), pen);
		}
	}
}

void GlDrawContext::clipRectangle(const rect2f& rct)
{
	VALIDATE_CTX();
	auto size = viewport().size;
	auto normRect = Impl::asGlNormalize(rct, size);

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
	return Impl::viewport();
}

}

