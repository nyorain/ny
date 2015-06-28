#pragma once

#include <ny/include.hpp>
#include <ny/util//nonCopyable.hpp>

#include <functional>

namespace ny
{

class appContext : public nonCopyable
{
public:
    appContext();
    virtual ~appContext();

    virtual bool mainLoopCall() = 0;
    virtual bool mainLoopNonBlocking(){ return mainLoopCall(); }

    virtual void startDataOffer(dataSource& source, const image& img){}
    virtual bool isOffering() const { return 0; }
    virtual void endDataOffer(){}

    virtual dataOffer* getClipboard(){ return nullptr; };
    virtual void setClipboard(dataSource& source){}

    //data specifications
    void setClipboard(const std::string& str);
    void setClipboard(const image& str);

    virtual void exit(){}
};

}
