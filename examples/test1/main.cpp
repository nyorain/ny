#include <iostream>
#include <ny/ny.hpp>

int main()
{
	ny::app myApp;

	if(myApp.init())
	{
		std::cout << "could not init. exit" << std::endl;
		//return 1;
	}

	ny::toplevelWindow win(ny::vec2ui(100, 100), ny::vec2ui(800, 500), "Test");
	win.show();

	win.onDraw([](ny::drawContext& dc){
			dc.clear(ny::color::white);
			});

	return myApp.mainLoop();
}
