# ny

A lightweight and modern C++14 windowing toolkit. Finally.
Desined to be modular instead of monolithic, lightweight instead of bloated, modern instead
of supportive.
Licensed under the __Boost License__ (similar to MIT and BSD license but does not require 
attribution when only used in binaries).

Instead of writing one big pile of shit, we focused on writing modular components that can be
useful for writing applications with user interfaces. The ny infrastrcuture is therefore
splitted in 3 modular parts that can be used seperatly:
- evg: A drawing library for vector graphics with hardware acclerated backends
- ny: A cross-platform window and context creation/management library
- <guilib>(name needed): A library that abstracts user interfaces.

As soon as C++17 will be released and supported by the first compilers, ny will require it.
The code is written against the C++ standard and not against compiler features, therefore 
compiling ny with any mscv will not work, since it does not have full C++14 support.
(To be honest it does already depend on C++17, it uses std::any).

At the moment, ny is in a pre-alpha state, but the first alpha is expected to be released soon.

## Library

ny is splitted into three libraries:
- ny-base: really small utility library
	- contains most of the small abstract base classes like Event or EventHandler
	- most of the headers located in ny/base are self-contained and do not need the library

- ny-backend: the core of ny. Implements the platform window abstraction. Depends on ny-base.
	- contains interfaces like WindowContext or AppContext and different implementations for it.
	- can be used by applications that just want lightweight cross-platform window creation.

- ny-app: higher level wrapper around ny-backend. Depends on ny-backend and ny-base.
	- contains classes like Window, App, Dialog or Mouse.
	- can be used by applications that prefer a full-sized windowing toolkit

Note that many features can be used without having to link to any library.
Examples are interface (or mostly interface) classes like AppContext, WindowContext, Backend, 
Event or EventHandler. This way, it should be really easy to integrate with ny even if a
program or library does not want to use most of its features.

## Usage

Two simple examples to give you an idea on how using ny works. Both examples simply show
a basic window.
The first example uses the higher abstraction (ny-app).
```cpp
#include <ny/app.hpp>

int main()
{
	///Just create an app with default settings
	ny::App app;

	///Create a window for that app
	ny::Window window;

	///Run the apps main loop.
	app.run();
}
```

This example uses only the lower abstraction (ny-backend) and is therefore a bit more complex.
For normal gui applications, one usually should use the higher abstraction. Using this lower
abstraction might make sence for full control and performance optimization or if most of the
higher abstraction featuers are not needed. This could be the case if one just wants to create
a window into which can be rendered using vulkan or opengl.

```cpp
#include <ny/backend.hpp>

int main()
{
	///Choose an available backend.
	auto& backend = ny::Backend::choose();

	///Create an app context. Responsible for dispatching events and managing windows.
	auto appContext = backend.createAppContext();

	///Create a windowContext. Represents a window on the underlaying backend.
	///One can manually handle the events that are generated from this WindowContext.
	auto windowContext = appContext.createWindowContext();

	///The more complex part: creating an EventDispatcher.	
	///An EventDispatcher is responsible for dispatching generated events to their handlers.
	///For multithreaded applications, one could use a ThreadedEventDispatcher.
	ny::EventDispatcher dispatcher;

	///A LoopControl can be used to control a loop from inside it through callbacks
	///or from other threads.
	ny::LoopControl loopControl;

	///Just run the appContext on the given dispatcher forever.
	appContext->dispatchLoop(dispatcher, loopControl);
}
```

## Dependencies

There are a few dependencies that are shipped with ny which are header-only or will be 
automatically built if they are not found on the system.

- evg: Drawing library abstraction. Pulls in its own dependencies.
- stb_image/stb_image_write/stb_image_resize: lightweight C files for handling images.
- glad: [optional] gl bindings generator.

At the moment ny contains 3 implemented backends. 
All of those backends have different dependencies that have to be installed if one wants to built
ny with support for those backends.

- wayland
	- xkbcommon
	- wayland client and wayland client protocol
	- [optional] gl/gles for gl support.
	- [optional] egl and wayland-egl for gl support.
	- [optional] vulkan for vulkan support.
	- [optional] cairo for cairo support.
- x11
	- xkbcommon
	- xcb (icccm, ewmh and xkb)
	- xlib
	- [optional] gl/gles and glx for gl support.
	- [optional] vulkan for vulkan support.
	- [optional] cairo for cairo support.
- winapi
	- winapi and gdi headers
	- [optional] gl/gles for gl support.
	- [optional] vulkan for vulkan support.

Implementers for a mir, android or osx backend are highly appreciated.
ny has different dependencies on the different backends.

## Building ny

Just use cmake.
There are various options to configure the building process.
