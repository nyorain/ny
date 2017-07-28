# ny

A lightweight and modern C++14 windowing toolkit.
Licensed under the __Boost License__ (similar to MIT and BSD license but does not require
attribution when only used in binaries).

**_Under Construction_**

For some first sketchup of how ny will/could look, see the first [docs](doc) or some
really basic [example apps](src/examples).
The examples are of course available under public domain.
All kinds of contributions are appreciated.

### Building on windows

Building it on windows can be a bit tricky.
Try to use the following meson command:

```
meson build/win --layout=flat -Denable_winapi=true -Denable_x11=false -Denable_wayland=false -Dvulkan_sdk=%VULKAN_SDK% -Dexamples=true --default-library=static
```

Or disable vulkan with ```-Denable_vulkan=false``` when you don't have the vulkan sdk
installed.
