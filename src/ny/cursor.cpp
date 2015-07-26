#include <ny/cursor.hpp>
#include <ny/image.hpp>

namespace ny
{

cursor::cursor()
{
}

cursor::cursor(cursorType t) : type_(t), image_(nullptr)
{
}

cursor::cursor(image& data) : type_(cursorType::unknown), image_(&data)
{
}


void cursor::fromImage(image& data)
{
    image_ = &data;
    hotspot_ = - (data.getSize() / 2);

    type_ = cursorType::unknown;
}

void cursor::fromImage(image& data, vec2i hotspot)
{
    image_ = &data;
    hotspot_ = hotspot;

    type_ = cursorType::unknown;
}

void cursor::fromNativeType(cursorType t)
{
    type_= t;
    image_ = nullptr;
}

bool cursor::isImage() const
{
    return (!image_);
}

bool cursor::isNativeType() const
{
    return (type_ != cursorType::unknown);
}

image* cursor::getImage() const
{
    return image_;
}

vec2i cursor::getImageHotspot() const
{
    return hotspot_;
}

cursorType cursor::getNativeType() const
{
    return type_;
}

}
