#include <ny/gl/triangulate.hpp>

#include <nyutil/line.hpp>
#include <nyutil/vec.hpp>

namespace ny
{

//atm: ear-clipping-algorithm,
//todo: maybe something more efficient
//      able to perform it in 3d space
//      able to handle holes

namespace
{
    thread_local std::vector<segment2> segments;
    thread_local std::vector<triangle2> triangles;
}

std::size_t findNextEar()
{
    for(std::size_t i(0); i < segments.size(); i++)
    {
        std::size_t next = (i + 1 == segments.size()) ? 0 : i + 1;
        bool found = 0;

        if(angle(segments[i].getDifference(), segments[next].getDifference()) / cDeg > 180.f)
        {
            triangle2 test(segments[i].a, segments[i].b, segments[next].b);
            found = 1;

            for(std::size_t o(0); o < segments.size(); o++)
            {
                if(o == i || o == next) continue;
                if(test.contains(segments[o].a))
                {
                    found = 0;
                    break;
                }
            }

            if(found)
            {
                return i;
            }
        }
    }

    return 0;
}

std::vector<triangle2> triangulate(float* points, std::size_t size)
{
    //init
    segments.reserve(size);
    triangles.reserve(size - 2);

    for(std::size_t i(0); i < size * 2; i += 2)
    {
        segments[i / 2].a = vec2f(points[i], points[i + 1]);
        segments[i / 2].b = vec2f(points[i + 2], points[i + 3]);
    }

    //iterate
    std::size_t i(0);
    while(segments.size() > 3)
    {
        std::size_t ear = findNextEar();
        triangles[i] = triangle2(segments[ear].a, segments[ear].b, segments[ear + 1].b);
        segments.erase(segments.begin() + ear);
        i++;
    }

    triangles.back() = triangle2(segments[0].a, segments[1].a, segments[2].a);

    //return
    return triangles;
}


}
