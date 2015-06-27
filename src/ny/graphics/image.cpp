#include <ny/graphics/image.hpp>

#include <ny/graphics/CImg.h>
using namespace cimg_library;

namespace ny
{

//wrapper class
class imageHandle 
{
public:
	CImg<unsigned char> img;
};

//image
image::image() : file(), surface(), handle_(0)
{	
	handle_ = new imageHandle;
}

image::image(const std::string& path) : file(path), surface()
{	
	handle_ = new imageHandle;
	handle_->img.load(path.c_str());
}

image::~image()
{
	if(handle_)
		delete handle_;
}

const unsigned char* image::getData() const
{
	return handle_->img.data();	
}

unsigned char* image::getData()
{
	return handle_->img.data();	
}

void image::setData(unsigned char* data)
{
}

vec2ui image::getSize() const
{
	return vec2ui(handle_->img.width(), handle_->img.height());
}

}
