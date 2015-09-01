#pragma once

#include <ny/include.hpp>
#include <ny/drawContext.hpp>
#include <ny/shape.hpp>

#include <memory>

namespace ny
{

bool isExtensionSupported(const char* extList, const char* extension);
bool validGLContext();

enum class glApi
{
    openGL,
    openGLES
};

//glDrawImplementation/////////////////////////////////////////////////////777
class glDrawImpl
{
public:
    virtual void clear(color col) = 0;
    virtual void fill(const mask& m, const brush& b) = 0;
    virtual void stroke(const mask& m, const pen& b) = 0;
};

//legacy
class legacyGLDrawImpl : public glDrawImpl
{
public:
    virtual void clear(color col) override;
    virtual void fill(const mask& m, const brush& b) override;
    virtual void stroke(const mask& m, const pen& b) override;
};

//gles
class glesDrawImpl : public glDrawImpl
{
public:
    virtual void clear(color col) override;
    virtual void fill(const mask& m, const brush& b) override;
    virtual void stroke(const mask& m, const pen& b) override;
};


//glDrawContext////////////////////////////////////////////////////////////////////////////77
class glDrawContext : public drawContext
{
protected:
    glApi api_{glApi::openGL};

    unsigned int depth_ {0};
    unsigned int stencil_ {0};

    unsigned int major_ {0};
    unsigned int minor_ {0};

    void init(glApi api, unsigned int depth = 0, unsigned int stencil = 0); //should be called at the end of constructor
    virtual bool makeCurrentImpl() = 0;
    virtual bool makeNotCurrentImpl() = 0;

    virtual bool assureValid(bool sendWarn = 1) const;

    //dc
    std::unique_ptr<glDrawImpl> impl_{nullptr};
    ny::mask store_{};

    rect2f clip_{};

public:
    glDrawContext(surface& s);
    virtual ~glDrawContext();

    //dc
    virtual void clear(color col = color::none) override;

    virtual void mask(const customPath& obj) override;
    virtual void mask(const text& obj) override;
    virtual void mask(const ny::mask& obj) override;
	virtual void mask(const path& obj) override;

    virtual void resetMask() override;

	virtual void fillPreserve(const brush& col) override;
	virtual void strokePreserve(const pen& col) override;

    virtual rect2f getClip() override;
    virtual void clip(const rect2f& obj) override;
	virtual void resetClip() override;

    //gl
    void updateViewport(vec2ui size);

    glApi getApi() const { return api_; }
    int getMajorVersion() const { return major_; }
    int getMinorVersion() const { return minor_; }

    unsigned int getDepthBits() const { return depth_; }
    unsigned int getStencilBits() const { return stencil_; }

    bool makeCurrent(); //specifed
    bool makeNotCurrent(); //specified
    bool isCurrent() const;

    virtual bool swapBuffers() = 0;

private:
    static thread_local glDrawContext* current_; //every thread can have 1 current context, todo: just use threadlocal?

protected:
    static void makeContextCurrent(glDrawContext& ctx);
    static void makeContextNotCurrent(glDrawContext& ctx);

public:
    static glDrawContext* getCurrent();
};

}
