# ny

<h2> Introduction </h2>
ny is a modern c++11 cross-platform gui toolkit. It uses modern c++ features like templates, namespaces, non-trival and type-safe unions, enum classes, lambdas, c++11 concurrency and other new stl features e.g. std::function or std::chrono instead of relying on macros and custom implementations.

At the moment, ny is in a pre-alpha version and under heavy developement, <b> it is not ready to use. </b>
The winapi backend is broken, there is no os x backend (but there will be porbably one in the future) and the linux backends are really unstable and missing lots of features.


<h2> Installation </h2>
Clone ny from this repository and cd to the toplevel-folder of it (.../ny/).

Buidling ny with cmake and make:
`````````````Bash
mkdir build && cd build
cmake ..
make
make install
`````````````

Alternativley you can use gnu Makefiles to build it (might not work):
``````````````Bash
cmake -DCMAKE_INSTALL_PREFIX=/usr
make
make install
``````````````

<h3> Dependencies </h3>
The dependencies for ny may differ for each platform and backend. On Linux you don't have to build it with X11, if you just want to build it with the Wayland backend (or even without any backend) it's fine, too. Then you have to set the according option when configuring cmake e.g. -DNY_WithX11=OFF

Like the backends, most of the other dependecies are optional, too. You can use cairo or OpenGL to draw on windows, FreeType to load and render fonts, but you don't have to.

Here is a list for the optional or needed backends and dependencies on different platforms:

<b> Windows: </b> winapi, gdi, openGL, cairo, freetype <br>
<b> Linux: </b> X11, wayland, cairo, EGL, openGL, openGL ES, freetype <br>
<b> OS X: </b> No OS X Backend for the moment <br>


<h2> Example </h2>
This basic example demonstrates how to create a toplevelWindow in just a few lines.

`````````````cpp
#include <ny/ny.hpp>

int main()
{
  ny::App app;
  ny::Frame frame(app, ny::Vec2ui(800, 500), "Hello World");

  frame.onDraw = [](ny::DrawContext& dc){ dc.clear(ny::Color::red); };

  return app.run();
}
````````````

The line in which we specify the draw-procedure for the window `frame.onDraw = ...` shows the possibilities of c++11 features like std::function. Where other gui-libraries rely on custom implemented or preprocessed signal-slot methods, ny simply uses callbacks. You could also specify a member function of your own class in `frame.onDraw`.
