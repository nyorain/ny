#pragma once

#include <ny/backend/include.hpp>
#include <nytl/nonCopyable.hpp>

namespace ny
{

class AppContext : public nonCopyable
{
public:
    AppContext() = default;
    virtual ~AppContext() = default;

	virtual int mainLoop() = 0;
	virtual void exit() {}

    //data
	/*
    virtual void startDataOffer(dataSource& source, const image& img, const window& w, const event* ev){}
    virtual bool isOffering() const { return 0; }
    virtual void endDataOffer(){}

    virtual dataOffer* getClipboard(){ return nullptr; };
    virtual void setClipboard(dataSource& source, const event* ev){}

    //data specifications
    void setClipboard(const std::string& str);
    void setClipboard(const image& str);
	*/
};

}
