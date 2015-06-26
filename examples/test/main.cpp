#include <iostream>
#include <ny/ny.hpp>
#include <ny/config.h>
//#include <ny/backends/backend.hpp>
//#include <ny/backends/x11/backend.hpp>

int main()
{
	std::cout << "hw" << std::endl;
#ifdef WithWayland
	std::cout << "Wayland" << std::endl;
#endif
#ifdef WithX11
	std::cout << "X11" << std::endl;
#endif

	ny::appSettings settings;
	settings.allBackends = 1;
	settings.backendThread = 0;
	settings.threadSafe = 1;
	settings.onError = ny::errorAction::AskConsole;

	ny::app myApp;

	if(myApp.init())
	{
		std::cout << "could not init. exit" << std::endl;
		//return 1;
	}

	ny::toplevelWindow win(ny::vec2ui(100, 100), ny::vec2ui(100, 100), "TEST");
	win.show();

	win.onDraw([](ny::drawContext& dc){
			dc.clear(ny::color::blue);
			});

	return myApp.mainLoop();
}
