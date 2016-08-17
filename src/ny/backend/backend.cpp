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

///wayland > x11 > winapi
Backend& Backend::choose()
{
	static const std::string waylandString = "wayland";
	static const std::string x11String = "x11";
	static const std::string winapiString = "winapi";

	auto bestScore = 0u;
	Backend* best = nullptr;
	for(auto& backend : backends())
	{
		if(!backend->available()) continue;

		auto score = 0u;
		if(backend->name() == waylandString) score = 3u;
		else if(backend->name() == x11String) score = 2u;
		else if(backend->name() == winapiString) score = 1u;

		if(score > bestScore)
		{
			bestScore = score;
			best = backend;
		}
	}

	if(!best) throw std::logic_error("ny::Backend: no backend available.");
	return *best;
}

}
