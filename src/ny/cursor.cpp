#include <ny/cursor.hpp>

namespace ny
{

cursor::cursor() : type_(cursorType::LeftPtr), image_(nullptr)
{
}

cursor::cursor(cursorType t) : type_(t), image_(nullptr)
{
}

cursor::cursor(image& data) : type_(cursorType::Unknown), image_(&data)
{
}


void cursor::fromImage(image& data)
{
    image_ = &data;
    type_ = cursorType::Unknown;
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
    return (type_ != cursorType::Unknown);
}

image* cursor::getImage() const
{
    return image_;
}

cursorType cursor::getNativeType() const
{
    return type_;
}

}
