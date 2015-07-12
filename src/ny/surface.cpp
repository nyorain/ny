#include <ny/surface.hpp>

namespace ny
{

unsigned int getBufferFormatSize(bufferFormat format)
{
    switch(format)
    {
        case bufferFormat::argb8888: return 4;
        case bufferFormat::xrgb8888: return 4;
        case bufferFormat::rgba8888: return 4;
        case bufferFormat::rgb888: return 3;
        case bufferFormat::bit: return 1;
        default: return 0;
    }
}

}
