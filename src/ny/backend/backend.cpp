#include <ny/backend/backend.hpp>

namespace ny
{

std::vector<Backend*> Backend::backendsFunc(Backend* reg, bool remove)
{
	static std::vector<Backend*> backends_;

	if(reg)
	{
		if(remove)
		{
			for(auto it = backends_.cbegin(); it < backends_.cend(); ++it)
			{
				if(*it == reg)
				{
					backends_.erase(it);
					break;
				}
			}
		}
		else
		{
			backends_.push_back(reg);
		}
	}

	return backends_;
}

}
