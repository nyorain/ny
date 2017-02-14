ny on android
=============

compiling for android is currently only working on linux (for me)
pull requests appreciated
use the official ndk toolchain file (the newest version) and cmake 3.6 (3.7 does not work somehow)
also use clang and the clang stl since we need c++17 support (clang 3.8 is ok)
work in progress
care: toolchains can only be specified the first time cmake is run in a binary dir

for an example see src/ny/examples
the example will generate a fully functional apk file from the
cross-platform example cpp when compiling for android. For the magic behind this see
src/cmake/android/apk.cmake. This is developed independent from ny
and can be used in your projects (see the example cmake).

set Android=1 to enable ny android support
platform is needed for vulkan support
we link to libc++ for c++14 support
the undefined symbols are needed for the extern main decl (cant this done better??)

```
cmake -G Ninja ../.. \
    -DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk/build/cmake/android.toolchain.cmake \
    -DAndroid=1 -DDepend=0 -DANDROID_STL=c++_static \
    -DANDROID_ALLOW_UNDEFINED_SYMBOLS=1 -DANDROID_CPP_FEATURES="rtti exceptions" \
	-DANDROID_PLATFORM=android-24 -DExamples=1
```

for custom architectures, use -DANDROID_ABI from the android toolchain:

- default (automatically chosen) -DANDROID_ABI=armeabi
- x86 (e.g. chromebook): -DANDROID_ABI=x86
- 64 bit (only for new devices): -DANDROID_ABI=arm64-v8a
