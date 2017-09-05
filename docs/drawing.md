Drawing on windows with ny
==========================

**_Since ny is only a window abstraction it does not mandate or implement some drawing library
to be used with it. It only abstracts the way an application can draw to a window._**

On WindowContext creation, the application can request some kind of surface that should be created
for/with the WindowContext in the passed WindowSettings.
Those surfaces cannot be created anymore once the WindowContext is created.

At the moment, there are 3 different types of surfaces the application can create and
use for a WindowContext:

	- vulkan surface
	- gl surface
	- raw pixel buffer surface for software rendering

If ny was built without vulkan/gl, creating these surfaces will fail.
The raw pixel buffer surface does not need any general extra dependencies but might
still not be supported on all backends (usually is).

If an application wants to draw onto a ny::WindowContext using e.g. toolkits like cairo, skia
or nanovg, it requests the needed/preferred surface on WindowContext creation and then connects it
with the tookit (e.g. create a cairo image surface for a raw pixel buffer or a vulkan swapchain
for the returned vulkan surface).

Below are the basic snippets for creating and then using such surfaces, for full example apps
see [src/examples](ny/tree/master/src/examples).

Vulkan
------

*Creating a vulkan surface for a ny window.*

Since ny is not a general utility toolkit but rather a platform abstraction for displaying
stuff and receiving user events, it does not provide some huge vulkan functionality.
It only implements the single vulkan-feature that is platform-dependent: surfaces.
Using ny::WindowSettings one can specify that a VkSurfaceKHR should be created for a
WindowContext. It must pass the VkInstance that the surface should be created for.

*__Note__*: The VkInstance must have been created with the needed extensions, otherwise
surface creation will fail. Those extensions can be queried using ny::AppContext::vulkanExtensions
for the associated AppContext.

```cpp
//first check that vulkan is theoretically supported
//backend is here the ny backend the appContext was created from (retrieved e.g.
//from ny::Backend::choose)
if(!backend.vulkan()) return -1;

//create a vulkan instance that supported the extensions needed to create a vulkan surface
//on the chosen backend
auto vkInstance = createInstanceForExtensions(appContext.vulkanExtensions());

//create windowSettings that request to create a VkSurfaceKHR for the WindowContext
//we also request to store the created surface (if successful) in vkSurface.
VkSurfaceKHR vkSurface {};

auto windowSettings = ny::WindowSettings {};
windowSettings.surface = ny::SurfaceType::vulkan;
windowSettings.vulkan.instance = vkInstance;
windowSettings.vulkan.storeSurface = reinterpret_cast<std::uintptr_t*>(&vkSurface);

auto windowContext = appContext.createWindowContext(windowSettings);

//Now we can use the created vulkan surface to render onto windowContext
//vkSurface is guaranteed to be valid until windowContext is destroyed
//note that the application should NOT destroy vkSurface manually.
```

*__Note__*: the explicit reintrpret_cast<std::uintptr_t*>(&vkSurface) is needed because ny does not
forward-declare the rather non-trivial type definition of VkSurfaceKHR (which is dependent on
whehter the compiler is 32 or 64 bit). Every occurrence of VkSurfaceKHR in ny is represented
by a std::uintptr_t since this is big enough to store its value on any platform.

OpenGL (ES)
-----------

*Rendering on a ny window using OpenGL (ES)*.

Using ny to render onto a WindowContext using opengl requires some more steps since huge
parts of opengl are platform dependent.
We have to create WindowContext with a GlSurface and then a GlContext.
This GlContext can then be made current for the created GlSurface and the opengl api
can be used to render onto the window.

Although gl itself does not abstract a "surface" in any way one can imagine it the same
way vulkan or cairo surfaces are used. They represent just some thing that can be rendered
on in some way (in vulkan using a swapchain, in cairo using a cairo_t, or in
opengl using a context).

```cpp
//first check that gl is supported
//backend is here the ny backend the appContext was created from (retrieved e.g.
//from ny::Backend::choose)
if(!backend.gl() || !appContext.glSetup()) return -1;

//create windowSettings that request to create a GlSurface for the WindowContext
//we also request to store the created GlSurface (if successful) in glSurface.
ny::GlSurface* glSurface {};

auto windowSettings = ny::WindowSettings {};
windowSettings.surface = ny::SurfaceType::gl;
windowSettings.gl.storeSurface = &glSurface;

auto windowContext = appContext.createWindowContext(windowSettings);

//now we have to create a GlContext to render onto the surface
//we could now specify the contexts settings using GlContextSettings, but
//here we just use the (sane) default settings
auto glContext = appContext.glSetup()->createContext();

//now we can make glContext current for glSurface
//after this call we can use the gl api to render onto windowContext
//this call may fail due to various reasons (mainly in multithreaded environments) and to
//manually handle errors, we could use the makeCurrent overload using a std::error_code
//This call will throw if it cannot be executed correctly
glContext->makeCurrent(*glSurface);

//Now we can use gl calls like glClearColor and glClear to clear the window.
//Note that glSurface is guaranteed to be valid at least until windowContxt is destructed
//glContext on the other side is currently wrapped into a std::unique_ptr and managed by
//the application
```

The code above is just some really basic sample. Gl applications could e.g. choose custom
configs for surface and context, could specify context api version and settings or enable
vsync for the current surface or create multiple shared contexts that can be used
in multiple threads.

BufferSurface
-------------

*Drawing onto the window using sofware rendering*

Creating a BufferSurface works similiar to gl and vulkan except that created surface
can be used to directly access the windows contents.

```cpp
//This time we do not have to check if the backend does explicitly support buffer surfaces
//or if the appContext is suitable to create those since every backend does support them by default

//create windowSettings that request to create a BufferSurface for the WindowContext
//we also request to store the created BufferSurface (if successful) in bufferSurface.
ny::BufferSurface* bufferSurface {};

auto windowSettings = ny::WindowSettings {};
windowSettings.surface = ny::SurfaceType::buffer;
windowSettings.buffer.storeSurface = &bufferSurface;

auto windowContext = appContext.createWindowContext(windowSettings);

//Now we can use the buffer surface to retrieve and access (e.g. draw into) a raw pixel buffer
//which contents will be displayed on the window.
//First we have to request a BufferGuard object that guards the drawing process.
//Imagine retrieving a BufferGuard object as constructing a raw pixel buffer and destructing
//the BufferGuard (e.g. at the end of the scope) as painting to pixel buffer onto
//the window. It is now guaranteed though that the content will not be applied before
//BufferGuard is destructed
//Note how we use an extra scope for it
{
	auto bufferGuard = bufferSurface->buffer();
	auto buffer = bufferGuard.get(); //decltype(buffer): ny::MutableImageData
	drawBuffer(buffer.data, buffer.format, buffer.size, buffer.stride);
}

//Note that bufferSurface is valid at least until windowContext is destructed.
```
