#include <ny/image.hpp>

#define cimg_use_png 1
#include <CImg.h>
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
image::image() : file(), surface(), handle_(nullptr)
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

imageType image::getType() const
{
    return imageType::Unknown;
}

const unsigned char* image::getDataPlain() const
{
    return handle_->img.data();
}

unsigned char* image::getDataPlain()
{
	return handle_->img.data();
}

//todo
const unsigned char* image::getDataConvent() const
{
    return handle_->img.data();
}

unsigned char* image::getDataConvent()
{
	return handle_->img.data();
}

vec2ui image::getSize() const
{
	return vec2ui(handle_->img.width(), handle_->img.height());
}

bufferFormat image::getBufferFormat() const
{
    return bufferFormat::Unknown;
}

bool image::saveToFile(const std::string& path) const
{
    handle_->img.save(path.c_str());
    return 1;
}

bool image::loadFromFile(const std::string& path)
{
    handle_->img.load(path.c_str());
    return 1;
}

}