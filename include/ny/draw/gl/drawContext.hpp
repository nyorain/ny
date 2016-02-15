#pragma once

#include <ny/include.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/draw/gl/context.hpp>
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
			Shader textureRGBA;
			Shader textureRGB;
			Shader colorTextureA;
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
	Vec2f asGlInvert(const Vec2f& point, float ySize = 1);
	Rect2f asGlInvert(const Rect2f& rct, float ySize = 1);

	Vec2f asGlNormalize(const Vec2f& point, const Vec2f& size);
	Rect2f asGlNormalize(const Rect2f& rct, const Vec2f& size);

	Vec2f asGlCoords(const Vec2f& point, const Vec2f& size);
	Rect2f asGlCoords(const Rect2f& point, const Vec2f& size);

	ShaderPrograms& shaderPrograms();
	Shader& shaderProgramForBrush(const Brush& b);
	Shader& shaderProgramForPen(const Pen& b);

	void fillTriangles(const std::vector<Triangle2f>&, const Brush&, const Mat3f& = {});
	void strokePath(const std::vector<Vec2f>& points, const Pen& b, const Mat3f& = {});
	void fillText(const Text& t, const Brush& b);
	void strokeText(const Text& t, const Pen& p);

public:
	virtual void clear(const Brush& brush) override;
	virtual void paint(const Brush& alpha, const Brush& fill) override;

	virtual void fillPreserve(const Brush& brush) override;
	virtual void strokePreserve(const Pen& pen) override;

	virtual bool maskClippingSupported() const override { return 0; }

	virtual void clipRectangle(const Rect2f& rct) override;
	virtual Rect2f rectangleClip() const override;
	virtual void resetRectangleClip() override;

	virtual void apply() override;

	//gl-specific
	virtual void viewport(const Rect2f& viewport);
	Rect2f viewport() const;
};

}
