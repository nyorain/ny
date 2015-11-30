#include <ny/image.hpp>

#if (!defined NY_WithWinapi)
#define cimg_use_png 1
#endif //NOT Winapi

//#include "CImg.h"
//using namespace cimg_library;

namespace ny
{

//wrapper class
class image::impl
{
public:
	//CImg<unsigned char> img;
};

//image
image::image() : file(), surface(), impl_(nullptr)
{
	//impl_ = make_unique<impl>();
}

image::image(const std::string& path) : file(path), surface()
{
	//impl_ = make_unique<impl>();
	//impl_->img.load(path.c_str());
}

image::~image()
{
}

image::image(const image& other) : file(), surface(), impl_(nullptr)
{
	//impl_ = make_unique<impl>();
	//impl_->img = other.impl_->img;
}

image& image::operator=(const image& other)
{
	//impl_->img = other.impl_->img;
	//return *this;
}

const unsigned char* image::getDataPlain() const
{
    //return impl_->img.data();
}

unsigned char* image::getDataPlain()
{
	//return impl_->img.data();
}

unsigned int image::getBufferSize() const
{
    //return impl_->img.size();
}

//todo
unsigned char* image::getData() const
{
    /*
    unsigned char* ret = new unsigned char[impl_->img.size()]; //todo: store

    cimg_forXYC(impl_->img, x, y, c)
    {
        unsigned int pos = (impl_->img.width() * y + x) * impl_->img.spectrum() + c;
        ret[pos] = impl_->img(x,y,c);
    }

    return ret;
    */
}

void image::getData(unsigned char* data) const
{
    /*
    cimg_forXYC(impl_->img, x, y, c)
    {
        unsigned int pos = (impl_->img.width() * y + x) * impl_->img.spectrum() + c;
        data[pos] = impl_->img(x,y,c);
    }
    */
}

vec2ui image::getSize() const
{
	//return vec2ui(impl_->img.width(), impl_->img.height());
}

bufferFormat image::getBufferFormat() const
{
    //return bufferFormat::unknown;
}

unsigned int image::getStride() const
{
    //return getSize().x * impl_->img.spectrum();
}

bool image::save(std::ostream& os) const
{
    //impl_->img.save(path.c_str());
    //return 1;
}

bool image::load(std::istream& is)
{
    //impl_->img.load(path.c_str());
   // return 1;
}

}
