#pragma once

#include <memory>

namespace ny
{

//XXX: may be better named LoopControlInterface? it is no implementation...
///Abstract interface, implemented by the different backend AppContexts.
class LoopControlImpl
{
public:
	virtual void stop() = 0;
};

///Can be used to control potentially infinite running loops from the inside or another thread.
///Some backend functions that will wait for further events allow the caller to pass a LoopControl
///object as parameter which can then be used to stop the loop (e.g. to exit the application
///when a window is closed).
///It holds just a LoopControlImpl pointer that will be set by the called function to
///an implementation that can be used to stop their loop.
class LoopControl
{
public:
	~LoopControl() { stop(); }
	
	std::unique_ptr<LoopControlImpl> impl_ {nullptr};
	void stop(){ if(impl_) impl_->stop(); }	
};

}
