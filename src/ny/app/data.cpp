#include <ny/app/data.hpp>

namespace ny
{

void DataTypes::addType(unsigned char type)
{
	if(contains(type)) return;
	types.push_back(type);
}

void DataTypes::removeType(unsigned char type)
{
    auto it = types_.begin();
    while(it != types_.end())
    {
        if(*it == type)
        {
           types_.erase(it);
           return;
        }
        ++it;
    }
}

bool DataTypes::contains(unsigned char type) const
{
    auto it = types_.begin();
    while(it != types_.end())
    {
        if(*it == type) return true;
        it++;
    }

    return false;
}

//
unsigned char stringToDataType(const std::string& type)
{
    using namespace dataType;
    return 0;
}

std::vector<std::string> dataTypeToString(unsigned char type, bool onlyMime)
{
    std::vector<std::string> ret;
    return ret;
}

std::vector<std::string> dataTypesToString(DataTypes types, bool onlyMime)
{
    std::vector<std::string> ret;
    return ret;
}

}
