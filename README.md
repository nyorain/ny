# ny

<h2> Introduction </h2>
ny is a modern c++11 cross-platform gui toolkit. It tries to use modern c++ features like templates, namespaces, type-safety, enum classes, lambdas, c++11 concurrency and other new stl features e.g. std::function or std::chrono instead of relying on macros and custom implementations.

At the moment, ny is in a pre-alpha and under heavy developement, <b> it is not ready to use. </b>
The windows backend is broken at the moment, there is no os x backend (but there will be porbably one in the future) and the linux backends are really unstable and have not yet implemented most of the functionality they should have.


<h2> Installation </h2>
ny is built with cmake and ninja.

`````````````
cd ny/
mkdir build
cd build
cmake -G "Ninja" -DCMAKE_INSTALL_PREFIX=/usr ..
ninja
ninja install
```````````

<h3> Dependencies </h3>
The dependencies for ny may differ for each platform and backend. On Linux you don't have to build it with X11, if you just want to build it with the Wayland backend (or even without any backend) it's fine too. Then you have to set the accordingly option when configuring cmake e.g. -DNY_WITH_X11=OFF

Like the backends, most of the other dependecies are optional, too. You can e.g. use cairo or OoenGL to draw on Windows, but you don't have to, you can use gdi, too.

Here is a list for the optional or needed backends and dependencies on different platforms:

<b> Windows: </b> winapi, gdi, openGL, cairo
<b> Linux: </b> X11, Wayland, cairo, EGL, openGL, openGL ES
<b> OS X: </b> No OS X Backend for the moment


<h2> Example </h2>
This basic example demonstrate how to create a toplevelWindow in just a few lines.

`````````````
#include <ny/ny.hpp>
using namespace ny;

int main()
{
  app myApp;
  
  if(!app.init())
  {
    return 0;
  }
  
  toplevelWindow win(vec2i(100, 100), vec2ui(800, 500), "Hello World");
  win. show();
  
  return myApp.mainLoop();
}
````````````
