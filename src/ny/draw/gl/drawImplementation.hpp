#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/gl/glDrawContext.hpp>
#include <ny/draw/gl/shader.hpp>

#include <nytl/vec.hpp>
#include <nytl/rect.hpp>

namespace ny
{

///Implements the actual OpenGL(ES) drawing
struct GlDrawContext::Impl
{
	static vec2f asGlInvert(const vec2f& point, float ySize = 1);
	static rect2f asGlInvert(const rect2f& rct, float ySize = 1);

	static vec2f asGlNormalize(const vec2f& point, const vec2f& size);
	static rect2f asGlNormalize(const rect2f& rct, const vec2f& size);

	static vec2f asGlCoords(const vec2f& point, const vec2f& size);
	static rect2f asGlCoords(const rect2f& point, const vec2f& size);

	static ShaderPrograms& shaderPrograms();
	static Shader& shaderProgramForBrush(const Brush& b);
	static Shader& shaderProgramForPen(const Pen& b);

	static bool modern();

	static void fillTrianglesModern(const std::vector<triangle2f>&, const Brush&, const mat3f&);
	static void fillTrianglesLegacy(const std::vector<triangle2f>&, const Brush& b, const mat3f&);

	static void strokePathModern(const std::vector<vec2f>& points, const Pen& pen, const mat3f&);
	static void strokePathLegacy(const std::vector<vec2f>& points, const Pen& pen, const mat3f&);

	static void fillTextModern(const Text& t, const Brush& b);
	static void fillTextLegacy(const Text& t, const Brush& b);

	static void strokeTextModern(const Text& t, const Pen& b);
	static void strokeTextLegacy(const Text& t, const Pen& b);

	static void fillTriangles(const std::vector<triangle2f>&, const Brush&, const mat3f& = {});
	static void strokePath(const std::vector<vec2f>& points, const Pen& b, const mat3f& = {});
	static void fillText(const Text& t, const Brush& b);
	static void strokeText(const Text& t, const Pen& p);

	static rect2f viewport();
};

///Holds all shader programs a GLDC instance needs to draw.
struct GlDrawContext::ShaderPrograms
{
	bool initialized = 0;

	struct
	{
		Shader color;
		Shader texture;
		Shader radialGradient;
		Shader linearGradient;
	} brush;

	struct
	{
		Shader color;
		Shader texture;
		Shader radialGradient;
		Shader linearGradient;
	} pen;
};

}
