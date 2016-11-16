//developement test for inline-only functionaility
//mainly useful while working on interfaces and therefore breaking the library source

#include <ny/log.hpp>
#include <vector>

namespace ny
{

using MyComplexName = int;

struct A
{
	void a(MyComplexName somename = 8)
	{
		NY_ERROR("i am just a random error message.. ", 7, " or ", 453, "?");
		// constexpr auto b = __PRETTY_FUNCTION__;
		// static_assert(b[0] == 's', "");
	}
};

}

int main()
{
	ny::A b;
	b.a();
}
