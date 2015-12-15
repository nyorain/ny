#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/draw/gl/glContext.hpp>

namespace ny
{

///OpenGL(ES) draw context implementation.
class GlDrawContext : public DelayedDrawContext
{
public:
	class Impl;
	struct ShaderPrograms;
	static ShaderPrograms& shaderPrograms();

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
