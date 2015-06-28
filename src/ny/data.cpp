#include <ny/data.hpp>

namespace ny
{

void dataTypes::addType(unsigned char type)
{
	if(type == dataType::allImage)
	{
		types_.push_back(dataType::image::bmp);
		types_.push_back(dataType::image::tiff);
		types_.push_back(dataType::image::png);
		types_.push_back(dataType::image::jpeg);
		types_.push_back(dataType::image::gif);
		types_.push_back(dataType::image::svg);
		return;
	}

    types_.push_back(type);
}

void dataTypes::removeType(unsigned char type)
{
    std::vector<unsigned char>::iterator it = types_.begin();
    while(it != types_.end())
    {
        if(*it == type)
        {
           types_.erase(it);
           return;
        }
        it++;
    }
}

bool dataTypes::contains(unsigned char type) const
{
    std::vector<unsigned char>::const_iterator it = types_.begin();
    while(it != types_.end())
    {
        if(*it == type) return 1;
        it++;
    }

    return 0;
}

/////////////////////////////////////////////////////////////
unsigned char stringToDataType(const std::string& type)
{
    using namespace dataType;

    if(type == "image/png") return image::png;
    if(type == "image/jpeg") return image::jpeg;
    if(type == "image/gif") return image::gif;

    if(type == "text/plain") return dataType::text;

    return 0;
}

std::vector<std::string> dataTypeToString(unsigned char type, bool onlyMime)
{
    std::vector<std::string> ret;
    return ret;
}

std::vector<std::string> dataTypesToString(dataTypes types, bool onlyMime)
{
    std::vector<std::string> ret;
    return ret;
}

}
