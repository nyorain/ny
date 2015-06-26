#pragma once

#include <ny/include.hpp>
#include <ny/utils/nonCopyable.hpp>

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

    //virtual void startDataOffer(const image& img, const dataObject& offer){}
    //virtual bool isOffering(){ return 0; }
    //virtual void endDataOffer(){}

    //virtual bool getClipboard(dataTypes types, std::function<void(dataObject*)> callback){};
    //virtual void setClipboard(dataObject& obj){ }

    virtual void exit(){}
};

}
