#include <ny/draw/triangulate.hpp>

#include <nytl/line.hpp>
#include <nytl/vec.hpp>

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
    float angle;
};

//
namespace
{
    std::vector<ppoint> points;
    std::vector<triangle2f> triangles;
    bool clockwise;
    float fullangle;
}


//
std::size_t nextPoint(std::size_t i)
{
    return (i == points.size() - 1) ? 0 : i + 1;
}

std::size_t prevPoint(std::size_t i)
{
    return (i == 0) ? points.size() - 1 : i - 1;
}


float updateAngle(std::size_t i)
{
    vec2f l1(points[i].pos - points[prevPoint(i)].pos);
    vec2f l2(points[nextPoint(i)].pos - points[i].pos);

    points[i].angle = cangle(l1, l2) / cDeg;
    fullangle += (points[i].pos.x - points[prevPoint(i)].pos.x) * 
		(points[i].pos.y + points[prevPoint(i)].pos.y);

    return points[i].angle;
}

bool isConvex(std::size_t i)
{
    return (clockwise) ? (points[i].angle >= 180.f) : (points[i].angle < 180.f);
}

vec<3, std::size_t> findNextEar()
{
    for(std::size_t i(0); i < points.size(); i++)
    {
        std::size_t prev = prevPoint(i);
        std::size_t next = nextPoint(i);

        if(isConvex(i)) //if point is convex
        {
            triangle2f test(points[prev].pos, points[i].pos, points[next].pos);

            bool found = 1;
            for(std::size_t o(0); o < points.size(); o++)
            {
                if(isConvex(o) || o == i || o == next || o == prev) continue;
                if(contains(test, points[o].pos))
                {
                    found = 0;
                    break;
                }
            }

            if(found) return {prev, i, next};
        }
    }

    //nyDebug("triangulate failed");
    return {0,0,0}; //should never occur
}

std::vector<triangle2f> triangulate(const std::vector<vec2f>& xpoints)
{
    //init
    points.clear();
    triangles.clear();

    points.resize(xpoints.size());
    triangles.resize(xpoints.size() - 2);

    fullangle = 0;
    for(std::size_t i(0); i < xpoints.size(); i++)
    {
        points[i].pos = xpoints[i];
        if(i != 0)
		{
			updateAngle(i - 1);
		}
    }
    updateAngle(points.size() - 1);

    if(fullangle > 0) clockwise = 0;
    else clockwise = 1;

    //iterate
    std::size_t i(0);
    while(points.size() > 3)
    {
        vec<3, std::size_t> ear = findNextEar();
        triangles[i] = triangle2f(points[ear.x].pos, points[ear.y].pos, points[ear.z].pos);
        //nyDebug("c: ", i, " ", ear.x, " ", ear.y, " ", ear.z, " ", triangles[i]);
        points.erase(points.begin() + ear.y);

        updateAngle(ear.x);
        updateAngle(ear.z > 0 ? ear.z - 1 : 0);

        i++;
    }
	//nyDebug("some debug");

    triangles[i] = triangle2f(points[0].pos, points[1].pos, points[2].pos); //triangles.back()
    points.clear();

    return triangles;
}



}
