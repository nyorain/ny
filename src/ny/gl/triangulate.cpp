#include <ny/gl/triangulate.hpp>

#include <nyutil/line.hpp>
#include <nyutil/vec.hpp>

namespace ny
{

//atm: ear-clipping-algorithm,
//todo: maybe something more efficient
//      able to perform it in 3d space
//      able to handle holes

//
struct ppoint
{
    vec2f pos;
    bool convex;
};

//
namespace
{
    thread_local std::vector<vec2f> points;
    thread_local std::vector<triangle2f> triangles;
    bool clockwise;
}


//
std::size_t nextPoint(std::size_t i)
{

}

std::size_t prevPoint(std::size_t i)
{

}


bool updateConvex(std::size_t i)
{

}

vec<3, std::size_t> findNextEar()
{
    for(std::size_t i(0); i < points.size(); i++)
    {
        std::size_t next = (i + 1 == points.size()) ? 0 : i + 1;
        std::size_t prev = (i == 0) ? points.size() - 1 : i - 1;

        vec2f l1(points[i] - points[prev]);
        vec2f l2(points[next] - points[i]);
        float angl = (atan2(l1.y, l1.x) - atan2(l2.y, l2.x)) / cDeg;

        if(angl < 180.f) //if point is convex
        {
            triangle2f test(points[prev], points[i], points[next]);
            bool found = 1;

            for(std::size_t o(0); o < points.size(); o++)
            {
                if(o == i || o == next || o == prev) continue;
                if(test.contains(points[o]))
                {
                    found = 0;
                    break;
                }
            }

            if(found)
            {
                return {prev, i, next};
            }
        }
    }

    return {0,0,0}; //should never occur
}

std::vector<triangle2f> triangulate(float* xpoints, std::size_t size)
{
    //init
    points.clear();
    triangles.clear();

    points.resize(size);
    triangles.resize(size - 2);

    for(std::size_t i(0); i < size * 2; i += 2)
    {
        points[i / 2].x = xpoints[i];
        points[i / 2].y = xpoints[i + 1];
    }

    //todo: init convex/concave points store
    //      check if points are counter- or clockwise
    //      holes

    //iterate
    std::size_t i(0);
    while(points.size() > 3)
    {
        std::cout << "it: " << i << " s: " << points.size() << std::endl;
        vec<3, std::size_t> ear = findNextEar();
        triangles[i] = triangle2f(points[ear.x], points[ear.y], points[ear.z]);
        points.erase(points.begin() + ear.y);
        i++;
    }

    triangles[i] = triangle2f(points[0], points[1], points[2]); //triangles.back()
    points.clear();

    //return
    return triangles;
}



}
