#include <ny/base/data.hpp>

namespace ny
{

void DataTypes::add(unsigned int type)
{
	if(contains(type)) return;
	types.push_back(type);
}

void DataTypes::remove(unsigned int type)
{
    auto it = types.begin();
    while(it != types.end())
    {
        if(*it == type)
        {
           types.erase(it);
           return;
        }
        ++it;
    }
}

bool DataTypes::contains(unsigned int type) const
{
	for(auto t : types) if(t == type) return true;
    return false;
}

//
unsigned int stringToDataType(const std::string& type)
{
    using namespace dataType;
    return 0;
}

std::vector<std::string> dataTypeToString(unsigned int type, bool onlyMime)
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
