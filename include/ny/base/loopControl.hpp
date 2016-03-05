#pragma once

#include <memory>

namespace ny
{

//
class LoopControlImpl
{
public:
	virtual void stop() = 0;
};

//
class LoopControl
{
public:
	~LoopControl() { stop(); }
	
	std::unique_ptr<LoopControlImpl> impl_ {nullptr};
	void stop(){ if(impl_) impl_->stop(); }	
};

}
