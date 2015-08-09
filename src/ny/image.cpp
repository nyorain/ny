#include <ny/image.hpp>

#if (!defined NY_WithWinapi)
#define cimg_use_png 1
#endif //NOT Winapi

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

const unsigned char* image::getDataPlain() const
{
    return handle_->img.data();
}

unsigned char* image::getDataPlain()
{
	return handle_->img.data();
}

unsigned int image::getBufferSize() const
{
    return handle_->img.size();
}

//todo
unsigned char* image::getDataConvent() const
{
    unsigned char* ret = new unsigned char[handle_->img.size()];

    cimg_forXYC(handle_->img, x, y, c)
    {
        unsigned int pos = (handle_->img.width() * y + x) * handle_->img.spectrum() + c;
        ret[pos] = handle_->img(x,y,c);
    }

    return ret;
}

void image::getDataConvent(unsigned char* data) const
{
    cimg_forXYC(handle_->img, x, y, c)
    {
        unsigned int pos = (handle_->img.width() * y + x) * handle_->img.spectrum() + c;
        data[pos] = handle_->img(x,y,c);
    }
}

vec2ui image::getSize() const
{
	return vec2ui(handle_->img.width(), handle_->img.height());
}

bufferFormat image::getBufferFormat() const
{
    return bufferFormat::unknown;
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

std::unique_ptr<drawContext> image::getDrawContext()
{
    std::unique_ptr<drawContext> ret;
    return ret;
}

}
