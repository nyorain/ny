#pragma once

#include "include.hpp"
#include <mutex>
#include <thread>
#include <map>
#include <utils/nonCopyable.hpp>

namespace ny
{

bool isExtensionSupported(const char* extList, const char* extension);

enum class glApi
{
    openGL,
    openGLES
};

class glContext : public nonCopyable
{
protected:
    glApi api_;
    int major_;
    int minor_;

    unsigned int depth_;
    unsigned int stencil_;

    modernGLContext* modern_ = nullptr;
    legacyGLContext* legacy_ = nullptr;

    glContext();
    void init(glApi api, unsigned int depth, unsigned int stencil);

public:
    virtual ~glContext();

    glApi getApi() const { return api_; }
    int getMajorVersion() const { return major_; }
    int getMinorVersion() const { return minor_; }

    unsigned int getDepthBits() const { return depth_; }
    unsigned int getStencilBits() const { return stencil_; }

    virtual bool makeCurrent() = 0; //specifed
    virtual bool makeNotCurrent() = 0; //specified
    bool isCurrent();

    modernGLContext* getModernGL() { return modern_; }
    legacyGLContext* getLegacyGL() { return legacy_; }

//static
private:
    //every thread can have 1 current context
    static std::map<std::thread::id, glContext*> current_;

protected:
    static void makeContextCurrent(glContext& ctx);
    static void makeContextNotCurrent(glContext& ctx);

public:
    static bool valid();
    static glContext* getCurrent();
};

inline bool validGLContext(){ return glContext::valid(); }
inline glContext* currentGLContext(){ return glContext::getCurrent(); }


//specifications
////////////////////////////
class modernGLContext
{
protected:
    glContext& context_;

public:
    modernGLContext(glContext& c) : context_(c) {}
    glContext& getGLContext() const { return context_; }

};

////////////////////////////
class legacyGLContext
{
protected:
    glContext& context_;

public:
    legacyGLContext(glContext& c) : context_(c) {}
    glContext& getGLContext() const { return context_; }
};

}
