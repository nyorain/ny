#pragma once

#include <ny/include.hpp>
#include <ny/drawContext.hpp>

namespace ny
{

//glDrawContext
class glDrawContext : public drawContext
{
protected:
    rect2f clip_;
    //glContext& context_;

public:
    glDrawContext(surface& s, glContext& ctx);
    glDrawContext(surface& s);

    void clear(color col = color::none);

    virtual void mask(const customPath& obj){};
    virtual void mask(const text& obj){};
    virtual void resetMask(){};

	virtual void fill(const brush& col){};
	virtual void outline(const pen& col){};

    virtual rect2f getClip();
    virtual void clip(const rect2f& obj);
	virtual void resetClip();
};


}
