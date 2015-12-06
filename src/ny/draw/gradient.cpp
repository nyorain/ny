#include <ny/draw/gradient.hpp>

namespace ny
{

void ColorGradient::addStop(const ColorGradient::Stop& stop)
{
    stops_.push_back(stop);
}

void ColorGradient::addStop(float position, const Color& col)
{
    stops_.emplace_back({position, col});
}

Color ColorGradient::colorAt(float position) const
{
    if(stops_.empty()) return Color::none;
    
    Stop* lastOne = nullptr;
    for(auto& s : stops_)
    {
        if(s.position > position)
        {
            if(!lastOne) return s.color;
            float distance = s.position - lastOne->position;

            float higherFac = (s.position - position) / distance;
            return higherFac * s.color + (1 - higherFac) * lastOne->color;
        }

        lastOne_ = &s;
    }

    return stops_.back().color;
}

}
