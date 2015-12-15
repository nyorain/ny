#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/gl/glDrawContext.hpp>
#include <ny/draw/gl/shader.hpp>

#include <nytl/vec.hpp>
#include <nytl/rect.hpp>

namespace ny
{

class GlDrawContext::Impl
{
public:
	static vec2f asGlInvert(const vec2f& point, float ySize = 1);
	static rect2f asGlInvert(const rect2f& rct, float ySize = 1);

	static vec2f asGlNormalize(const vec2f& point, const vec2f& size);
	static rect2f asGlNormalize(const rect2f& rct, const vec2f& size);

	static vec2f asGlCoords(const vec2f& point, const vec2f& size);
	static rect2f asGlCoords(const rect2f& point, const vec2f& size);

public:
	static Shader& shaderProgramForBrush(const Brush& b);
	static bool modern();

	static void fillTrianglesModern(const std::vector<triangle2f>&, const Brush&, const mat3f&);
	static void fillTrianglesLegacy(const std::vector<triangle2f>& triangles, const Brush& b);

	static void strokePathModern(const std::vector<vec2f>& points, const Pen& pen);
	static void strokePathLegacy(const std::vector<vec2f>& points, const Pen& pen);

	static void fillTextModern(const Text& t, const Brush& b);
	static void fillTextLegacy(const Text& t, const Brush& b);

	static void strokeTextModern(const Text& t, const Pen& b);
	static void strokeTextLegacy(const Text& t, const Pen& b);

	static void fillTriangles(const std::vector<triangle2f>&, const Brush&, const mat3f&);
	static void strokePath(const std::vector<vec2f>& points, const Pen& b);
	static void fillText(const Text& t, const Brush& b);
	static void strokeText(const Text& t, const Pen& p);

	static rect2f viewport();
};

struct GlDrawContext::ShaderPrograms
{
	bool initialized = 0;

	Shader color;
	Shader texture;
	Shader radialGradient;
	Shader linearGradient;
};


}
