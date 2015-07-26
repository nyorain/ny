#pragma once

#include <ny/include.hpp>
#include <ny/drawContext.hpp>

#include <map>
#include <thread>

namespace ny
{

bool isExtensionSupported(const char* extList, const char* extension);
bool validGLContext();

enum class glApi
{
    openGL,
    openGLES
};

//glDrawContext
class glDrawContext : public drawContext
{
protected:
    glApi api_;

    unsigned int depth_;
    unsigned int stencil_;

    unsigned int major_;
    unsigned int minor_;

    void init(glApi api, unsigned int depth = 0, unsigned int stencil = 0); //should be called at the end of constructor
    virtual bool makeCurrentImpl() = 0;
    virtual bool makeNotCurrentImpl() = 0;

    //dc
    rect2f clip_;

public:
    glDrawContext(surface& s);
    virtual ~glDrawContext();

    //dc
    virtual void clear(color col = color::none) override;

    virtual void mask(const customPath& obj) override {};
    virtual void mask(const text& obj) override {};
    virtual void resetMask() override {};

	virtual void fill(const brush& col) override {};
	virtual void outline(const pen& col) override {};

    virtual rect2f getClip() override;
    virtual void clip(const rect2f& obj) override;
	virtual void resetClip() override;

    //gl
    glApi getApi() const { return api_; }
    int getMajorVersion() const { return major_; }
    int getMinorVersion() const { return minor_; }

    unsigned int getDepthBits() const { return depth_; }
    unsigned int getStencilBits() const { return stencil_; }

    bool makeCurrent(); //specifed
    bool makeNotCurrent(); //specified
    bool isCurrent();

    virtual bool swapBuffers() = 0;

private:
    static std::map<std::thread::id, glDrawContext*> current_; //every thread can have 1 current context

protected:
    static void makeContextCurrent(glDrawContext& ctx);
    static void makeContextNotCurrent(glDrawContext& ctx);

public:
    static glDrawContext* getCurrent();
};

}
