Implementation notes
====================

*__Notes abouts different parts of the documentation, mainly interesting for ny devs__*

Backend-specific - x11
----------------------

ftp://www.x.org/pub/X11R7.7/doc/man/man3/xcb-requests.3.xhtml

Sources for implementing x11/data:

https://github.com/edrosten/x_clipboard/blob/master/paste.cc
https://github.com/edrosten/x_clipboard/blob/master/selection.cc
https://www.freedesktop.org/wiki/Specifications/XDND
https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#use_of_selection_atoms
https://www.irif.fr/~jch/software/UTF8_STRING/UTF8_STRING.text

#### Atoms:

The ny backend has to load lots of atoms from the x server since they are one of the main
element of the x spec. There are 3 main sources for x atoms:

	- predefined XCB_ATOM_* atoms (xproto.h)
	- atoms loaded by xcb-ewmh, directly accesed in the xcb_ewmh_connection_t
	- atoms loaded manually, stored in a x11::Atoms object by the X11AppContext

#### xdnd:

starting a dnd request:

	- create a preview drag window with type _NET_WM_WINDOW_TYPE_DND
		- preview the dragged content
	- grab the x cursor
	- send enter/position events to windows we hover over


Backend-specific - Wayland
--------------------------

Current cursor implementation (not optimal, to be changed):
- every WindowContext has its own wayland::ShmBuffer that is used for custom image cursors
- every WindowContext has a non-owned wl_buffer* that holds the cursor contents for its surface
- WaylandMouseContext has a wl_surface* that is always the cursor surface

Backend-specifi - Winapi
------------------------

About winapi, keycodes and unicode:
Winapi makes it very hard for ny to correctly convert keycodes to utf8 string.
To correctly implement the KeyboardContext::utf8 function, WinapiKeyboardContext holds
an internal table of keycodes mapped to their default unicode strings since querying this
when needed might interfer with the real key events.
The only free function for converting keycodes to unicode is ::ToUnicode which does not really
meet all needs.

Some posts regarding clearing the keyboard buffer:
http://web.archive.org/web/20101004154432/http://blogs.msdn.com/b/michkap/archive/2006/04/06/569632.aspx
http://web.archive.org/web/20100820152419/http://blogs.msdn.com/b/michkap/archive/2007/10/27/5717859.aspx

Android
-------

Sources for a potential ndk/NativeActivity-based android backend:

- native_window header:
https://android.googlesource.com/platform/frameworks/native/+/master/include/android/native_window.h
- ALooper documentation:
https://developer.android.com/ndk/reference/group___looper.html#gaa7cd0636edc4ed227aadc585360ebefa
- ndk example:
https://github.com/googlesamples/android-ndk/blob/master/native-activity/app/src/main/cpp/main.cpp
- native app glue header/source
http://www.srombauts.fr/android-ndk-r5b/sources/android/native_app_glue/
- native app glue impl
http://www.ikerhurtado.com/android-ndk-native-activity-app-glue-lib-lifecycle-threads
- android header
https://github.com/pfalcon/android-platform-headers/tree/master/android-6.0.0_r1/frameworks/native/include/android
- natvieActivity docs
https://developer.android.com/ndk/reference/group___native_activity.html#ga7b0652533998d61e1a3b542485889113
- sfml android implementation
ny should have something like sfml has. The android "backend" can be used just like
every other. One should (theoretically) be able to compile (and run) an application on linux and
then also run it on android.
https://github.com/SFML/SFML/blob/master/src/SFML/Main/MainAndroid.cpp
