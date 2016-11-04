// The MIT License (MIT)
//
// Copyright (c) 2013 David Herberth
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/*

    GLX loader generated by glad 0.1.10a0 on Thu May 26 18:04:06 2016.

    Language/Generator: C/C++
    Specification: glx
    APIs: glx=1.4
    Profile: -
    Extensions:
        GLX_ARB_create_context, GLX_ARB_create_context_profile, GLX_ARB_framebuffer_sRGB, GLX_ARB_multisample, GLX_EXT_create_context_es2_profile, GLX_EXT_create_context_es_profile, GLX_EXT_swap_control
    Loader: No

    Commandline:
        --api="glx=1.4" --generator="c" --spec="glx" --no-loader --extensions="GLX_ARB_create_context,GLX_ARB_create_context_profile,GLX_ARB_framebuffer_sRGB,GLX_ARB_multisample,GLX_EXT_create_context_es2_profile,GLX_EXT_create_context_es_profile,GLX_EXT_swap_control"
    Online:
        http://glad.dav1d.de/#language=c&specification=glx&api=glx%3D1.4&extensions=GLX_ARB_create_context&extensions=GLX_ARB_create_context_profile&extensions=GLX_ARB_framebuffer_sRGB&extensions=GLX_ARB_multisample&extensions=GLX_EXT_create_context_es2_profile&extensions=GLX_EXT_create_context_es_profile&extensions=GLX_EXT_swap_control
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ny/x11/glxApi.hpp>

static void* get_proc(const char *namez);

#ifdef _WIN32
#include <windows.h>
static HMODULE libGL;

typedef void* (APIENTRYP PFNWGLGETPROCADDRESSPROC_PRIVATE)(const char*);
PFNWGLGETPROCADDRESSPROC_PRIVATE gladGetProcAddressPtr;

static
int open_gl(void) {
    libGL = LoadLibraryA("opengl32.dll");
    if(libGL != NULL) {
        gladGetProcAddressPtr = (PFNWGLGETPROCADDRESSPROC_PRIVATE)GetProcAddress(
                libGL, "wglGetProcAddress");
        return gladGetProcAddressPtr != NULL;
    }

    return 0;
}

static
void close_gl(void) {
    if(libGL != NULL) {
        FreeLibrary(libGL);
        libGL = NULL;
    }
}
#else
#include <dlfcn.h>
static void* libGL;

#ifndef __APPLE__
typedef void* (APIENTRYP PFNGLXGETPROCADDRESSPROC_PRIVATE)(const char*);
PFNGLXGETPROCADDRESSPROC_PRIVATE gladGetProcAddressPtr;
#endif

static
int open_gl(void) {
#ifdef __APPLE__
    static const char *NAMES[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
    };
#else
    static const char *NAMES[] = {"libGL.so.1", "libGL.so"};
#endif

    unsigned int index = 0;
    for(index = 0; index < (sizeof(NAMES) / sizeof(NAMES[0])); index++) {
        libGL = dlopen(NAMES[index], RTLD_NOW | RTLD_GLOBAL);

        if(libGL != NULL) {
#ifdef __APPLE__
            return 1;
#else
            gladGetProcAddressPtr = (PFNGLXGETPROCADDRESSPROC_PRIVATE)dlsym(libGL,
                "glXGetProcAddressARB");
            return gladGetProcAddressPtr != NULL;
#endif
        }
    }

    return 0;
}

static
void close_gl() {
    if(libGL != NULL) {
        dlclose(libGL);
        libGL = NULL;
    }
}
#endif

static
void* get_proc(const char *namez) {
    void* result = NULL;
    if(libGL == NULL) return NULL;

#ifndef __APPLE__
    if(gladGetProcAddressPtr != NULL) {
        result = gladGetProcAddressPtr(namez);
    }
#endif
    if(result == NULL) {
#ifdef _WIN32
        result = (void*)GetProcAddress(libGL, namez);
#else
        result = dlsym(libGL, namez);
#endif
    }

    return result;
}

int gladLoadGLX(Display *dpy, int screen) {
    int status = 0;

    if(open_gl()) {
        status = gladLoadGLXLoader((GLADloadproc)get_proc, dpy, screen);
        close_gl();
    }

    return status;
}

static Display *GLADGLXDisplay = 0;
static int GLADGLXscreen = 0;

static int get_exts(void) {
    return 1;
}

static void free_exts(void) {
    return;
}

static int has_ext(const char *ext) {
    const char *terminator;
    const char *loc;
    const char *extensions;

    if(!GLAD_GLX_VERSION_1_1)
        return 0;

    extensions = glXQueryExtensionsString(GLADGLXDisplay, GLADGLXscreen);

    if(extensions == NULL || ext == NULL)
        return 0;

    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL)
            break;

        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0'))
        {
            return 1;
        }
        extensions = terminator;
    }

    return 0;
}

int GLAD_GLX_VERSION_1_0;
int GLAD_GLX_VERSION_1_1;
int GLAD_GLX_VERSION_1_2;
int GLAD_GLX_VERSION_1_3;
int GLAD_GLX_VERSION_1_4;
PFNGLXGETSELECTEDEVENTPROC glad_glXGetSelectedEvent;
PFNGLXQUERYEXTENSIONPROC glad_glXQueryExtension;
PFNGLXMAKECURRENTPROC glad_glXMakeCurrent;
PFNGLXSELECTEVENTPROC glad_glXSelectEvent;
PFNGLXCREATECONTEXTPROC glad_glXCreateContext;
PFNGLXCREATEGLXPIXMAPPROC glad_glXCreateGLXPixmap;
PFNGLXQUERYVERSIONPROC glad_glXQueryVersion;
PFNGLXGETCURRENTREADDRAWABLEPROC glad_glXGetCurrentReadDrawable;
PFNGLXDESTROYPIXMAPPROC glad_glXDestroyPixmap;
PFNGLXGETCURRENTCONTEXTPROC glad_glXGetCurrentContext;
PFNGLXGETPROCADDRESSPROC glad_glXGetProcAddress;
PFNGLXWAITGLPROC glad_glXWaitGL;
PFNGLXISDIRECTPROC glad_glXIsDirect;
PFNGLXDESTROYWINDOWPROC glad_glXDestroyWindow;
PFNGLXCREATEWINDOWPROC glad_glXCreateWindow;
PFNGLXCOPYCONTEXTPROC glad_glXCopyContext;
PFNGLXCREATEPBUFFERPROC glad_glXCreatePbuffer;
PFNGLXSWAPBUFFERSPROC glad_glXSwapBuffers;
PFNGLXGETCURRENTDISPLAYPROC glad_glXGetCurrentDisplay;
PFNGLXGETCURRENTDRAWABLEPROC glad_glXGetCurrentDrawable;
PFNGLXQUERYCONTEXTPROC glad_glXQueryContext;
PFNGLXCHOOSEVISUALPROC glad_glXChooseVisual;
PFNGLXQUERYSERVERSTRINGPROC glad_glXQueryServerString;
PFNGLXDESTROYCONTEXTPROC glad_glXDestroyContext;
PFNGLXDESTROYGLXPIXMAPPROC glad_glXDestroyGLXPixmap;
PFNGLXGETFBCONFIGATTRIBPROC glad_glXGetFBConfigAttrib;
PFNGLXUSEXFONTPROC glad_glXUseXFont;
PFNGLXDESTROYPBUFFERPROC glad_glXDestroyPbuffer;
PFNGLXCHOOSEFBCONFIGPROC glad_glXChooseFBConfig;
PFNGLXCREATENEWCONTEXTPROC glad_glXCreateNewContext;
PFNGLXMAKECONTEXTCURRENTPROC glad_glXMakeContextCurrent;
PFNGLXGETCONFIGPROC glad_glXGetConfig;
PFNGLXGETFBCONFIGSPROC glad_glXGetFBConfigs;
PFNGLXCREATEPIXMAPPROC glad_glXCreatePixmap;
PFNGLXWAITXPROC glad_glXWaitX;
PFNGLXGETVISUALFROMFBCONFIGPROC glad_glXGetVisualFromFBConfig;
PFNGLXQUERYDRAWABLEPROC glad_glXQueryDrawable;
PFNGLXQUERYEXTENSIONSSTRINGPROC glad_glXQueryExtensionsString;
PFNGLXGETCLIENTSTRINGPROC glad_glXGetClientString;
int GLAD_GLX_ARB_multisample;
int GLAD_GLX_ARB_framebuffer_sRGB;
int GLAD_GLX_EXT_create_context_es_profile;
int GLAD_GLX_ARB_create_context;
int GLAD_GLX_ARB_create_context_profile;
int GLAD_GLX_EXT_create_context_es2_profile;
int GLAD_GLX_EXT_swap_control;
PFNGLXCREATECONTEXTATTRIBSARBPROC glad_glXCreateContextAttribsARB;
PFNGLXSWAPINTERVALEXTPROC glad_glXSwapIntervalEXT;
static void load_GLX_VERSION_1_0(GLADloadproc load) {
	if(!GLAD_GLX_VERSION_1_0) return;
	glad_glXChooseVisual = (PFNGLXCHOOSEVISUALPROC)load("glXChooseVisual");
	glad_glXCreateContext = (PFNGLXCREATECONTEXTPROC)load("glXCreateContext");
	glad_glXDestroyContext = (PFNGLXDESTROYCONTEXTPROC)load("glXDestroyContext");
	glad_glXMakeCurrent = (PFNGLXMAKECURRENTPROC)load("glXMakeCurrent");
	glad_glXCopyContext = (PFNGLXCOPYCONTEXTPROC)load("glXCopyContext");
	glad_glXSwapBuffers = (PFNGLXSWAPBUFFERSPROC)load("glXSwapBuffers");
	glad_glXCreateGLXPixmap = (PFNGLXCREATEGLXPIXMAPPROC)load("glXCreateGLXPixmap");
	glad_glXDestroyGLXPixmap = (PFNGLXDESTROYGLXPIXMAPPROC)load("glXDestroyGLXPixmap");
	glad_glXQueryExtension = (PFNGLXQUERYEXTENSIONPROC)load("glXQueryExtension");
	glad_glXQueryVersion = (PFNGLXQUERYVERSIONPROC)load("glXQueryVersion");
	glad_glXIsDirect = (PFNGLXISDIRECTPROC)load("glXIsDirect");
	glad_glXGetConfig = (PFNGLXGETCONFIGPROC)load("glXGetConfig");
	glad_glXGetCurrentContext = (PFNGLXGETCURRENTCONTEXTPROC)load("glXGetCurrentContext");
	glad_glXGetCurrentDrawable = (PFNGLXGETCURRENTDRAWABLEPROC)load("glXGetCurrentDrawable");
	glad_glXWaitGL = (PFNGLXWAITGLPROC)load("glXWaitGL");
	glad_glXWaitX = (PFNGLXWAITXPROC)load("glXWaitX");
	glad_glXUseXFont = (PFNGLXUSEXFONTPROC)load("glXUseXFont");
}
static void load_GLX_VERSION_1_1(GLADloadproc load) {
	if(!GLAD_GLX_VERSION_1_1) return;
	glad_glXQueryExtensionsString = (PFNGLXQUERYEXTENSIONSSTRINGPROC)load("glXQueryExtensionsString");
	glad_glXQueryServerString = (PFNGLXQUERYSERVERSTRINGPROC)load("glXQueryServerString");
	glad_glXGetClientString = (PFNGLXGETCLIENTSTRINGPROC)load("glXGetClientString");
}
static void load_GLX_VERSION_1_2(GLADloadproc load) {
	if(!GLAD_GLX_VERSION_1_2) return;
	glad_glXGetCurrentDisplay = (PFNGLXGETCURRENTDISPLAYPROC)load("glXGetCurrentDisplay");
}
static void load_GLX_VERSION_1_3(GLADloadproc load) {
	if(!GLAD_GLX_VERSION_1_3) return;
	glad_glXGetFBConfigs = (PFNGLXGETFBCONFIGSPROC)load("glXGetFBConfigs");
	glad_glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)load("glXChooseFBConfig");
	glad_glXGetFBConfigAttrib = (PFNGLXGETFBCONFIGATTRIBPROC)load("glXGetFBConfigAttrib");
	glad_glXGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)load("glXGetVisualFromFBConfig");
	glad_glXCreateWindow = (PFNGLXCREATEWINDOWPROC)load("glXCreateWindow");
	glad_glXDestroyWindow = (PFNGLXDESTROYWINDOWPROC)load("glXDestroyWindow");
	glad_glXCreatePixmap = (PFNGLXCREATEPIXMAPPROC)load("glXCreatePixmap");
	glad_glXDestroyPixmap = (PFNGLXDESTROYPIXMAPPROC)load("glXDestroyPixmap");
	glad_glXCreatePbuffer = (PFNGLXCREATEPBUFFERPROC)load("glXCreatePbuffer");
	glad_glXDestroyPbuffer = (PFNGLXDESTROYPBUFFERPROC)load("glXDestroyPbuffer");
	glad_glXQueryDrawable = (PFNGLXQUERYDRAWABLEPROC)load("glXQueryDrawable");
	glad_glXCreateNewContext = (PFNGLXCREATENEWCONTEXTPROC)load("glXCreateNewContext");
	glad_glXMakeContextCurrent = (PFNGLXMAKECONTEXTCURRENTPROC)load("glXMakeContextCurrent");
	glad_glXGetCurrentReadDrawable = (PFNGLXGETCURRENTREADDRAWABLEPROC)load("glXGetCurrentReadDrawable");
	glad_glXQueryContext = (PFNGLXQUERYCONTEXTPROC)load("glXQueryContext");
	glad_glXSelectEvent = (PFNGLXSELECTEVENTPROC)load("glXSelectEvent");
	glad_glXGetSelectedEvent = (PFNGLXGETSELECTEDEVENTPROC)load("glXGetSelectedEvent");
}
static void load_GLX_VERSION_1_4(GLADloadproc load) {
	if(!GLAD_GLX_VERSION_1_4) return;
	glad_glXGetProcAddress = (PFNGLXGETPROCADDRESSPROC)load("glXGetProcAddress");
}
static void load_GLX_ARB_create_context(GLADloadproc load) {
	if(!GLAD_GLX_ARB_create_context) return;
	glad_glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)load("glXCreateContextAttribsARB");
}
static void load_GLX_EXT_swap_control(GLADloadproc load) {
	if(!GLAD_GLX_EXT_swap_control) return;
	glad_glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)load("glXSwapIntervalEXT");
}
static int find_extensionsGLX(void) {
	if (!get_exts()) return 0;
	GLAD_GLX_ARB_create_context = has_ext("GLX_ARB_create_context");
	GLAD_GLX_ARB_create_context_profile = has_ext("GLX_ARB_create_context_profile");
	GLAD_GLX_ARB_framebuffer_sRGB = has_ext("GLX_ARB_framebuffer_sRGB");
	GLAD_GLX_ARB_multisample = has_ext("GLX_ARB_multisample");
	GLAD_GLX_EXT_create_context_es2_profile = has_ext("GLX_EXT_create_context_es2_profile");
	GLAD_GLX_EXT_create_context_es_profile = has_ext("GLX_EXT_create_context_es_profile");
	GLAD_GLX_EXT_swap_control = has_ext("GLX_EXT_swap_control");
	free_exts();
	return 1;
}

static void find_coreGLX(Display *dpy, int screen) {
	int major = 0, minor = 0;
	if(dpy == 0 && GLADGLXDisplay == 0) {
		dpy = XOpenDisplay(0);
		screen = XScreenNumberOfScreen(XDefaultScreenOfDisplay(dpy));
	} else if(dpy == 0) {
		dpy = GLADGLXDisplay;
		screen = GLADGLXscreen;
	}
	glXQueryVersion(dpy, &major, &minor);
	GLADGLXDisplay = dpy;
	GLADGLXscreen = screen;
	GLAD_GLX_VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
	GLAD_GLX_VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
	GLAD_GLX_VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
	GLAD_GLX_VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
	GLAD_GLX_VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
}

int gladLoadGLXLoader(GLADloadproc load, Display *dpy, int screen) {
	glXQueryVersion = (PFNGLXQUERYVERSIONPROC)load("glXQueryVersion");
	if(glXQueryVersion == NULL) return 0;
	find_coreGLX(dpy, screen);
	load_GLX_VERSION_1_0(load);
	load_GLX_VERSION_1_1(load);
	load_GLX_VERSION_1_2(load);
	load_GLX_VERSION_1_3(load);
	load_GLX_VERSION_1_4(load);

	if (!find_extensionsGLX()) return 0;
	load_GLX_ARB_create_context(load);
	load_GLX_EXT_swap_control(load);
	return 1;
}
