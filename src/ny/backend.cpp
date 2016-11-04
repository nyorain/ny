#include <ny/backend.hpp>
#include <cstdlib>
#include <cstring>

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

Backend& Backend::choose()
{
	static const std::string waylandString = "wayland";
	static const std::string x11String = "x11";
	static const std::string winapiString = "winapi";

	auto* envBackend = std::getenv("NY_BACKEND");

	auto bestScore = -1;
	Backend* best = nullptr;
	for(auto& backend : backends())
	{
		if(!backend->available()) continue;
		if(envBackend && !std::strcmp(backend->name(), envBackend)) return *backend;

		//score is chosen this way since there might be x servers on winapi
		//but no winapi on linux and we always want the native backend
		//wayland > x11 because of Xwayland
		auto score = 0u;
		if(backend->name() == winapiString) score = 3u;
		else if(backend->name() == waylandString) score = 2u;
		else if(backend->name() == x11String) score = 1u;

		if(score > bestScore)
		{
			bestScore = score;
			best = backend;
		}
	}

	if(!best) throw std::runtime_error("ny::Backend: no backend available.");
	return *best;
}

}
