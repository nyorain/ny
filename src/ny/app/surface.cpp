#include <ny/app/ny/app/surface.hpp>

namespace ny
{

unsigned int getBufferFormatSize(bufferFormat format)
{
    switch(format)
    {
        case bufferFormat::argb8888: return 4;
        case bufferFormat::bit: return 1;
        default: return 0;
    }
}

}
