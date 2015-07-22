#pragma once

#include <ny/include.hpp>

#include <mutex>
#include <thread>
#include <map>
#include <ny/util/nonCopyable.hpp>

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

    unsigned int depth_;
    unsigned int stencil_;

    unsigned int major_;
    unsigned int minor_;

    glContext();
    void init(glApi api, unsigned int depth = 0, unsigned int stencil = 0); //should be called at the end of constructor

    virtual bool makeCurrentImpl() = 0;
    virtual bool makeNotCurrentImpl() = 0;

public:
    virtual ~glContext();

    glApi getApi() const { return api_; }
    int getMajorVersion() const { return major_; }
    int getMinorVersion() const { return minor_; }

    unsigned int getDepthBits() const { return depth_; }
    unsigned int getStencilBits() const { return stencil_; }

    bool makeCurrent(); //specifed
    bool makeNotCurrent(); //specified
    bool isCurrent();

    //todo
    virtual modernGLContext* getModernGL() const { return nullptr; };
    virtual legacyGLContext* getLegacyGL() const { return nullptr; };

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
//////////////
class modernGLContext
{
protected:
    glContext& context_;

public:
    modernGLContext(glContext& c) : context_(c) {}
    glContext& getGLContext() const { return context_; }

};

//////////////
class legacyGLContext
{
protected:
    glContext& context_;

public:
    legacyGLContext(glContext& c) : context_(c) {}
    glContext& getGLContext() const { return context_; }
};

}
