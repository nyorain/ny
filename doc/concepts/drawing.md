Drawing on windows with ny
==========================

**_Since ny is only a window abstraction it does not mandate or implement some drawing library
to be used with it. It only abstracts the way an application can draw to a window._**

On WindowContext creation, the application can request some special surface that should be created
for/with the WindowConetxt in the passed WindowSettings (e.g. gl or vulkan).
Those integrations cannot be created anymore once the WindowContext is created.
Other integrations (e.g. with drawing toolkits as cairo or skia) can be created an
any time for the WindowContext but there can be only one at a time.

The application can only request those intergrations if ny was built with suppport for them
and if the specific backend of the WindowContext supports integration for them.
Therefore the application can also ask ny to create a raw pixel buffer without any
drawing integration (called __BufferSurface__). The application can then render into some
retrieved pixel buffer with a drawing toolkit of choice or directly manipulate it somehow.

*__Note__*: BufferSurface does also count as drawing integration and if the
application has already created a BufferSurface for a WindowContext it cannot
create e.g. a CairoIntegration object for it.

Furthermore can BufferSurface creation fail as well e.g. when the backend has no
possibility to implement them. BufferSurface (and some other drawing integration) creation
will usually fail if the WindowContext was created with a specific context.
Exceptions for this are mainly drawing integrations for toolkits that can render using
gl or vulkan.
Therefore WindowContexts do not care about any kind of drawing, contexts, surfaces or
pixel buffers automatically, the application must explicitly request them.

Vulkan
------

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
auto vkSurface = vkSurfaceKHR {};

auto windowSettings = ny::WindowSettings {};
windowSettings.surface = ny::SurfaceType::vulkan;
windowSettings.vulkan.instance = vkInstance;
windowSettings.vulkan.storeSurface = &vkSurface;

auto windowContext = appContext.createWindowContext(windowSettings);
if(!vkSurface) return -1;

//Now we can use the created vulkan surface to render onto windowContext
//vkSurface is guaranteed to be valid until windowContext is destroyed
//note that the application should NOT destroy vkSurface manually.
```

OpenGL (ES)
-----------

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
auto glSurface = ny::GlSurface*;

auto windowSettings = ny::WindowSettings {};
windowSettings.surface = ny::SurfaceType::gl;
windowSettings.gl.storeSurface = &glSurface;

auto windowContext = appContext.createWindowContext(windowSettings);
if(!glSurface) return -1;

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
=============

```cpp
```
