#pragma once

#include <ny/draw/include.hpp>
#include <ny/draw/color.hpp>

#include <nytl/vec.hpp>
#include <nytl/rect.hpp>
#include <nytl/region.hpp>
#include <nytl/nonCopyable.hpp>

namespace ny
{

class Mask;

//drawContext
class DrawContext : public nonCopyable
{
public:
	DrawContext();
	virtual ~DrawContext();

	virtual void apply(){}
	virtual void clear(color = color::none);

	virtual void mask(const Mask& obj);
	virtual void mask(const Path& obj);
	virtual void resetMask() = 0;


    virtual void mask(const CustomPath& obj) = 0;
	virtual void mask(const Text& obj) = 0;

	virtual void mask(const Rectangle& obj);
	virtual void mask(const Circle& obj);


	virtual void fill(const Brush& col); //will clear mask
	virtual void stroke(const Pen& col); //will clear mask

	virtual void fillPreserve(const Brush& col) = 0;
	virtual void strokePreserve(const Pen& col) = 0;

    virtual Mask currentClip() const = 0;
    virtual void clip();
	virtual void clipPreserve() = 0;
	virtual void resetClip() = 0;

    virtual void draw(const Shape& obj);
};

//redirectDrawContext
class RedirectDrawContext : public DrawContext
{
protected:
	rect2f clipSave_;

	vec2f size_;
    vec2f position_;

    drawContext& redirect_;

public:
	redirectDrawContext(DrawContext& redirect, vec2ui position, vec2ui size);

	virtual void apply() override;
	virtual void clear(color col = color::none) override;

	virtual void mask(const customPath& obj) override;
	virtual void mask(const rectangle& obj) override;
	virtual void mask(const text& obj) override;
	virtual void resetMask() override;

	virtual void fillPreserve(const pen& col) override;
	virtual void strokePreserve(const brush& col) override;

    virtual rect2f getClip() override { return redirect_.getClip(); };
    virtual void clip(const rect2f& obj) override; 
	virtual void resetClip() override { redirect_.resetClip(); };

	void setSize(vec2f size);
	void setPosition(vec2d position);

    void startClip();
	void endClip();
};

}
