#pragma once

#include <ny/include.hpp>
#include <ny/color.hpp>

#include <nyutil/vec.hpp>
#include <nyutil/rect.hpp>
#include <nyutil/region.hpp>
#include <nyutil/nonCopyable.hpp>

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
	virtual void clear(color = color::none);

	virtual void mask(const ny::mask& obj);
	virtual void mask(const path& obj);
	virtual void resetMask() = 0;


    virtual void mask(const customPath& obj) = 0;
	virtual void mask(const text& obj) = 0;

	virtual void mask(const rectangle& obj);
	virtual void mask(const circle& obj);


	virtual void fill(const brush& col); //will clear mask
	virtual void stroke(const pen& col); //will clear mask

	virtual void fillPreserve(const brush& col) = 0;
	virtual void strokePreserve(const pen& col) = 0;

    //todo
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

	virtual void apply() override;
	virtual void clear(color col = color::none) override;

	virtual void mask(const customPath& obj) override;
	virtual void mask(const rectangle& obj) override;
	virtual void mask(const text& obj) override;
	virtual void resetMask() override;

	virtual void fillPreserve(const pen& col) override;
	virtual void strokePreserve(const brush& col) override;

    virtual rect2f getClip() override { return redirect_.getClip(); };
    virtual void clip(const rect2f& obj) override { redirect_.clip(rect2f(obj.position + position_, vec2f(std::min(obj.size.x, size_.x) - position_.x, std::min(obj.size.y, size_.y) - position_.y))); };
	virtual void resetClip() override { redirect_.resetClip(); };

	void setSize(vec2f size);
	void setPosition(vec2d position);

    void startClip();
	void updateClip();
	void endClip();
};

}
