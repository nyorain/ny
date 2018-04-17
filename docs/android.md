# How to get ny working for android

Requirements: meson, ninja, android ndk/sdk.
android sdk build-tools, an android platform and adb to install/debug.
Tested with android ndk r16b

## Create ndk toolchain

Go to ndk-root and run

```
./build/tools/make-standalone-toolchain.sh --arch=arm64 --platform=24 \
	--install-dir=/tmp/android --stl=libc++ --verbose
```

This will install a toolchain to /tmp/android

## Build ny and its dependencies

```
meson build/android --cross-file=./docs/android_cross.txt -Dexamples=true -Dandroid=true
```

## Create apk for example

Copy build.sh and AndroidManifest.xml to $builddir/src/examples.
Just follow the steps in build.sh (or execute it and debug what does
not work). Make sure to copy
 - libny.so
 - libdlg.so
 - libc++_shared.so
 - and the examples so
to lib/arm64-v8a. Then everything should work.

Copy the c++ lib from $android-ndk/sources/cxx-stl/llvm-libc++/libs/arm64-v8a/libc++_shared.so
