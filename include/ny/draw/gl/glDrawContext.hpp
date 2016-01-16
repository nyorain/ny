#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/draw/gl/glContext.hpp>
#include <ny/draw/gl/shader.hpp>

#include <nytl/vec.hpp>
#include <nytl/rect.hpp>
#include <nytl/mat.hpp>
#include <nytl/triangle.hpp>

#include <memory>
#include <map>

namespace ny
{

///OpenGL(ES) draw context implementation.
///\todo mask clipping
class GlDrawContext : public DelayedDrawContext
{
public:
	struct ShaderPrograms
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

public:
	vec2f asGlInvert(const vec2f& point, float ySize = 1);
	rect2f asGlInvert(const rect2f& rct, float ySize = 1);

	vec2f asGlNormalize(const vec2f& point, const vec2f& size);
	rect2f asGlNormalize(const rect2f& rct, const vec2f& size);

	vec2f asGlCoords(const vec2f& point, const vec2f& size);
	rect2f asGlCoords(const rect2f& point, const vec2f& size);

	ShaderPrograms& shaderPrograms();
	Shader& shaderProgramForBrush(const Brush& b);
	Shader& shaderProgramForPen(const Pen& b);

	void fillTriangles(const std::vector<triangle2f>&, const Brush&, const mat3f& = {});
	void strokePath(const std::vector<vec2f>& points, const Pen& b, const mat3f& = {});
	void fillText(const Text& t, const Brush& b);
	void strokeText(const Text& t, const Pen& p);

protected:
	static std::map<GlContext*, ShaderPrograms> shaderPrograms_;

public:
	virtual void clear(const Brush& brush) override;
	virtual void paint(const Brush& alpha, const Brush& fill) override;

	virtual void fillPreserve(const Brush& brush) override;
	virtual void strokePreserve(const Pen& pen) override;

	virtual bool maskClippingSupported() const override { return 0; }

	virtual void clipRectangle(const rect2f& rct) override;
	virtual rect2f rectangleClip() const override;
	virtual void resetRectangleClip() override;

	//gl-specific
	virtual void viewport(const rect2f& viewport);
	rect2f viewport() const;
};

}
