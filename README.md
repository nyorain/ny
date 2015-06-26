# ny

<h2> Introduction </h2>
ny is a modern c++11 cross-platform gui toolkit. It tries to use modern c++ features like templates, namespaces, type-safety, enum classes, lambdas, c++11 concurrency and other new stl features e.g. std::function or std::chrono instead of relying on macros and custom implementations.

At the moment, ny is in a pre-alpha and under heavy developement, <b> it is not ready to use. </b>


<h2> Installation </h2>
ny is built with cmake and ninja.

`````````````
cd ny/
mkdir -f build
cd build
cmake -G "Ninja" -DCMAKE_INSTALL_PREFIX=/usr ..
ninja
ninja install
```````````

<h2> Example </h2>
This basic example demonstrate how to create an app which creates a toplevelWindow in just a few lines.

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
