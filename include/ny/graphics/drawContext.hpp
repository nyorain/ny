#pragma once

#include <ny/include.hpp>
#include <ny/graphics/color.hpp>

#include <ny/utils/vec.hpp>
#include <ny/utils/rect.hpp>
#include <ny/utils/region.hpp>
#include <ny/utils/nonCopyable.hpp>

namespace ny
{

class mask;

//drawContext
class drawContext : public nonCopyable
{
protected:
	surface& surface_;
public:
	drawContext(surface& s);
	virtual ~drawContext();

	surface& getSurface() { return surface_; }
	const surface& getSurface() const { return surface_; }

	virtual void apply(){}
	virtual void clear(color = color::none) = 0;

	virtual void mask(const ny::mask& obj);
	virtual void mask(const path& obj) = 0;
	virtual void resetMask() = 0;

	virtual void fill(const brush& col) = 0;
	virtual void outline(const pen& col) = 0;

    virtual rect2f getClip() = 0;
    virtual void clip(const rect2f& obj) = 0;
	virtual void resetClip() = 0;

    virtual void draw(const shape& obj);
};

//redirectDrawContext
class redirectDrawContext : public drawContext
{
protected:
	rect2f clipSave_;

	vec2f size_;
    vec2f position_;

    drawContext& redirect_;

public:
	redirectDrawContext(drawContext& redirect, vec2f position, vec2f size);
    redirectDrawContext(drawContext& redirect, vec2f position = vec2f());

	virtual void apply();
	virtual void clear(color col = color::none);

	virtual void mask(const path& obj);
	virtual void resetMask();
	virtual void fill(const pen& col);
	virtual void outline(const brush& col);

    virtual rect2f getClip(){ return rect2f(); };
    virtual void clip(const rect2f& obj){};
	virtual void resetClip(){};

	void setSize(vec2d size);
	void setPosition(vec2d position);

    void startClip();
	void updateClip();
	void endClip();
};

}
