#pragma once

#include <ny/include.hpp>
#include <ny/util/nonCopyable.hpp>

#include <functional>

namespace ny
{

class appContext : public nonCopyable
{
public:
    appContext();
    virtual ~appContext();

    virtual bool mainLoop() = 0;
    virtual void exit(){} //may not be specified

    //egl
    #ifdef NY_WithEGL
    virtual eglAppContext* getEGLAppContext() const { return nullptr; }
    #endif //EGL

    //data
    virtual void startDataOffer(dataSource& source, const image& img, const window& w, const event* ev){}
    virtual bool isOffering() const { return 0; }
    virtual void endDataOffer(){}

    virtual dataOffer* getClipboard(){ return nullptr; };
    virtual void setClipboard(dataSource& source, const event* ev){}

    //data specifications
    void setClipboard(const std::string& str);
    void setClipboard(const image& str);
};

}
