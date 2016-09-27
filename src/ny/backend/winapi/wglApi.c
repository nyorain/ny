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

    WGL loader generated by glad 0.1.10a0 on Wed May 25 15:21:01 2016.

    Language/Generator: C/C++
    Specification: wgl
    APIs: wgl=1.0
    Profile: -
    Extensions:
        WGL_ARB_create_context, WGL_ARB_create_context_profile, WGL_ARB_framebuffer_sRGB, WGL_ARB_multisample, WGL_ARB_pbuffer, WGL_ARB_pixel_format, WGL_EXT_framebuffer_sRGB, WGL_EXT_swap_control, WGL_EXT_extensions_string, WGL_ARB_extensions_string
    Loader: False
    Local files: False
    Omit khrplatform: False

    Commandline:
        --api="wgl=1.0" --generator="c" --spec="wgl" --no-loader --extensions="WGL_ARB_create_context,WGL_ARB_create_context_profile,WGL_ARB_framebuffer_sRGB,WGL_ARB_multisample,WGL_ARB_pbuffer,WGL_ARB_pixel_format,WGL_EXT_framebuffer_sRGB,WGL_EXT_swap_control,WGL_EXT_extensions_string,WGL_ARB_extensions_string"
    Online:
        http://glad.dav1d.de/#language=c&specification=wgl&api=wgl%3D1.0&extensions=WGL_ARB_create_context&extensions=WGL_ARB_create_context_profile&extensions=WGL_ARB_framebuffer_sRGB&extensions=WGL_ARB_multisample&extensions=WGL_ARB_pbuffer&extensions=WGL_ARB_pixel_format&extensions=WGL_EXT_framebuffer_sRGB&extensions=WGL_EXT_swap_control&extensions=WGL_EXT_extensions_string&extensions=WGL_ARB_extensions_string
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ny/backend/winapi/wglApi.hpp>

static HDC GLADWGLhdc = (HDC)INVALID_HANDLE_VALUE;

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

    if(wglGetExtensionsStringEXT == NULL && wglGetExtensionsStringARB == NULL)
        return 0;

    if(wglGetExtensionsStringARB == NULL || GLADWGLhdc == INVALID_HANDLE_VALUE)
        extensions = wglGetExtensionsStringEXT();
    else
        extensions = wglGetExtensionsStringARB(GLADWGLhdc);

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
int GLAD_WGL_VERSION_1_0;
int GLAD_WGL_ARB_pixel_format;
int GLAD_WGL_ARB_pbuffer;
int GLAD_WGL_ARB_create_context_profile;
int GLAD_WGL_EXT_extensions_string;
int GLAD_WGL_ARB_create_context;
int GLAD_WGL_ARB_framebuffer_sRGB;
int GLAD_WGL_EXT_swap_control;
int GLAD_WGL_EXT_framebuffer_sRGB;
int GLAD_WGL_ARB_multisample;
int GLAD_WGL_ARB_extensions_string;
PFNWGLCREATECONTEXTATTRIBSARBPROC glad_wglCreateContextAttribsARB;
PFNWGLCREATEPBUFFERARBPROC glad_wglCreatePbufferARB;
PFNWGLGETPBUFFERDCARBPROC glad_wglGetPbufferDCARB;
PFNWGLRELEASEPBUFFERDCARBPROC glad_wglReleasePbufferDCARB;
PFNWGLDESTROYPBUFFERARBPROC glad_wglDestroyPbufferARB;
PFNWGLQUERYPBUFFERARBPROC glad_wglQueryPbufferARB;
PFNWGLGETPIXELFORMATATTRIBIVARBPROC glad_wglGetPixelFormatAttribivARB;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC glad_wglGetPixelFormatAttribfvARB;
PFNWGLCHOOSEPIXELFORMATARBPROC glad_wglChoosePixelFormatARB;
PFNWGLSWAPINTERVALEXTPROC glad_wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC glad_wglGetSwapIntervalEXT;
PFNWGLGETEXTENSIONSSTRINGEXTPROC glad_wglGetExtensionsStringEXT;
PFNWGLGETEXTENSIONSSTRINGARBPROC glad_wglGetExtensionsStringARB;
static void load_WGL_ARB_create_context(GLADloadproc load) {
	if(!GLAD_WGL_ARB_create_context) return;
	glad_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)load("wglCreateContextAttribsARB");
}
static void load_WGL_ARB_pbuffer(GLADloadproc load) {
	if(!GLAD_WGL_ARB_pbuffer) return;
	glad_wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)load("wglCreatePbufferARB");
	glad_wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)load("wglGetPbufferDCARB");
	glad_wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)load("wglReleasePbufferDCARB");
	glad_wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)load("wglDestroyPbufferARB");
	glad_wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARBPROC)load("wglQueryPbufferARB");
}
static void load_WGL_ARB_pixel_format(GLADloadproc load) {
	if(!GLAD_WGL_ARB_pixel_format) return;
	glad_wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)load("wglGetPixelFormatAttribivARB");
	glad_wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)load("wglGetPixelFormatAttribfvARB");
	glad_wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)load("wglChoosePixelFormatARB");
}
static void load_WGL_EXT_swap_control(GLADloadproc load) {
	if(!GLAD_WGL_EXT_swap_control) return;
	glad_wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)load("wglSwapIntervalEXT");
	glad_wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)load("wglGetSwapIntervalEXT");
}
static void load_WGL_EXT_extensions_string(GLADloadproc load) {
	if(!GLAD_WGL_EXT_extensions_string) return;
	glad_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)load("wglGetExtensionsStringEXT");
}
static void load_WGL_ARB_extensions_string(GLADloadproc load) {
	if(!GLAD_WGL_ARB_extensions_string) return;
	glad_wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)load("wglGetExtensionsStringARB");
}
static int find_extensionsWGL(void) {
	if (!get_exts()) return 0;
	GLAD_WGL_ARB_create_context = has_ext("WGL_ARB_create_context");
	GLAD_WGL_ARB_create_context_profile = has_ext("WGL_ARB_create_context_profile");
	GLAD_WGL_ARB_framebuffer_sRGB = has_ext("WGL_ARB_framebuffer_sRGB");
	GLAD_WGL_ARB_multisample = has_ext("WGL_ARB_multisample");
	GLAD_WGL_ARB_pbuffer = has_ext("WGL_ARB_pbuffer");
	GLAD_WGL_ARB_pixel_format = has_ext("WGL_ARB_pixel_format");
	GLAD_WGL_EXT_framebuffer_sRGB = has_ext("WGL_EXT_framebuffer_sRGB");
	GLAD_WGL_EXT_swap_control = has_ext("WGL_EXT_swap_control");
	GLAD_WGL_EXT_extensions_string = has_ext("WGL_EXT_extensions_string");
	GLAD_WGL_ARB_extensions_string = has_ext("WGL_ARB_extensions_string");
	free_exts();
	return 1;
}

static void find_coreWGL(HDC hdc) {
	GLADWGLhdc = hdc;
}

int gladLoadWGLLoader(GLADloadproc load, HDC hdc) {
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)load("wglGetExtensionsStringARB");
	wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)load("wglGetExtensionsStringEXT");
	if(wglGetExtensionsStringARB == NULL && wglGetExtensionsStringEXT == NULL) return 0;
	find_coreWGL(hdc);

	if (!find_extensionsWGL()) return 0;
	load_WGL_ARB_create_context(load);
	load_WGL_ARB_pbuffer(load);
	load_WGL_ARB_pixel_format(load);
	load_WGL_EXT_swap_control(load);
	load_WGL_EXT_extensions_string(load);
	load_WGL_ARB_extensions_string(load);
	return 1;
}
