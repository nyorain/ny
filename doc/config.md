Config
======

**_Since there are multiple possible build configurations for ny, applications can
deal differently with different ny configurations_**

There are two different ny configurations the application can check:

	- the configuration of the library it is linked to at compile time
	- the configuration of the library that is loaded at runteime (e.g. shared library)

Compiletime
------------

The first one is needed to determine which ny header can be included and which
functions be called without getting a compiler/linker error.
It looks e.g. like this:

```cpp
#include <ny/config.hpp>
#include <ny/fwd.hpp>

#ifdef NY_WithGl
	#include <ny/common/gl.hpp>
#endif

//Some application specfic functions that retrieves the current gl context if
//there is one.
ny::GlContext* currentGlContext()
{
	#ifdef NY_WithGl
		return ny::GlConetxt::current();
	#else
		return nullptr;
	#endif
}
```

Including ```<ny/common/gl.hpp>``` when ny was built without gl would trigger an error
explicitly specified in the header because using any function from the header will result
in a linking error.

Another important example would be dealing explicitly with a backends implementation
of some interface. This is only possible if ny was built with this backend since otherwise
the called symbol is simply not defined in the ny library which will trigger a linker error.

```cpp
#ifdef NY_WithX11
	auto x11wc = dynamic_cast<ny::X11WindowContext*>(windowContext.get());
	if(x11wc) x11wc->requestFocus();
#endif NY_WithX11
```

Without the compile time siwtch, the example above would refuse to compile/link on systems where ny was built
without x11 support, e.g. windows.

Alternatively applications might actively choose to just refuse to compile if they really depend on a
ny feature:

```cpp
#include <ny/config.hpp>
#ifndef NY_WithVulkan
	#error This application cannot be compiled using a ny version that was built without vulkan.
#endif //NY_WithVulkan
```

But this does not automatically assures that the ny library used at runtime does automatically support the
feature.

Runtime
-------

Since even when an application was built on a system that had a version on ny with some feature
installed, it might not be present on the system the application is actually executed on (since the machines
might be different or the ny installation is changed between compile time and execution time).
Therefore application should also check at runtime whether the needed feature is available.

This can either be done directly using the ny::with* (e.g. ny::withX11) function from
ny/config.hpp or using some functions that check this runtime config implicitly.

```cpp
//A general backend-independent way to check if the ny library of the executing system was built
//with gl is the ny::withGl() function defined in ny/config.hpp

//The first call checks whether the backend has theoretically gl support (which will return
//false if ny was built wihtout gl support at all).
//The second function will try to retrieve the ny::GlSetup* from the appContext.
//If this fails, gl initialization failed and therefore no gl surfaces or contexts can
//be created.
if(!backend.gl() || !appContext.glSetup())
{
	ny::error("Either the local ny library was built without gl support or gl init failed");
	ny::error("This application cannot be executed without ny gl support. Exiting");
	exit(EXIT_FAILURE);
}

auto glSurface = ny::GlSurface* {};

auto windowSettings = ny::WindowSettings {};
windowSettings.surface = ny::SurfaceType::gl;
windowSettings.gl.storeSurface = &glSurface;

auto windowContext = appContext.createWindowContext(windowSettings);
```

The application above does explicitly check whether ny supports gl (and gl initialization
succeeded). If not so it does output an error message for the user concretely describing the
error. Note that just because gl is supported and initialization succeeded, the createWindowContext
function might still fail (but not because gl is not initialized).

Note that in this case the application will not run on systems where ny was built
without gl support, so application can also simply switch the used drawing method depending
on what is available:

```cpp
auto glSurface = ny::GlSurface* {};
auto bufferSurface = ny::BufferSurface* {};
auto gl = false;

auto windowSettings = ny::WindowSettings {};

//Check again whether gl is supported and initialized
//But this time we don't error and fail if it isn't, but simply use a software
//renderer otherwise.
if(backend.gl() && appContext.glSetup())
{
	windowSettings.surface = ny::SurfaceType::gl;
	windowSettings.gl.storeSurface = &glSurface;

	gl = true;
}
else
{
	windowSettings.surface = ny::SurfaceType::buffer;
	windowSettings.buffer.storeSurface = &bufferSurface;
}

auto windowContext = appContext.createWindowContext(windowSettings);

if(gl) renderUsingGlSurface(glSurface);
else renderUsingBufferSurface(bufferSurface);
```
