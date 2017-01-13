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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ny/common/eglApi.h>

PFNEGLCREATEWINDOWSURFACEPROC glad_eglCreateWindowSurface;
PFNEGLQUERYCONTEXTPROC glad_eglQueryContext;
PFNEGLCREATEIMAGEPROC glad_eglCreateImage;
PFNEGLDESTROYSURFACEPROC glad_eglDestroySurface;
PFNEGLQUERYSTRINGPROC glad_eglQueryString;
PFNEGLINITIALIZEPROC glad_eglInitialize;
PFNEGLGETCURRENTDISPLAYPROC glad_eglGetCurrentDisplay;
PFNEGLGETSYNCATTRIBPROC glad_eglGetSyncAttrib;
PFNEGLSWAPBUFFERSPROC glad_eglSwapBuffers;
PFNEGLRELEASETEXIMAGEPROC glad_eglReleaseTexImage;
PFNEGLMAKECURRENTPROC glad_eglMakeCurrent;
PFNEGLTERMINATEPROC glad_eglTerminate;
PFNEGLCREATEPBUFFERSURFACEPROC glad_eglCreatePbufferSurface;
PFNEGLWAITCLIENTPROC glad_eglWaitClient;
PFNEGLDESTROYSYNCPROC glad_eglDestroySync;
PFNEGLGETCURRENTCONTEXTPROC glad_eglGetCurrentContext;
PFNEGLGETCONFIGSPROC glad_eglGetConfigs;
PFNEGLBINDTEXIMAGEPROC glad_eglBindTexImage;
PFNEGLCREATEPIXMAPSURFACEPROC glad_eglCreatePixmapSurface;
PFNEGLQUERYAPIPROC glad_eglQueryAPI;
PFNEGLWAITSYNCPROC glad_eglWaitSync;
PFNEGLCREATECONTEXTPROC glad_eglCreateContext;
PFNEGLGETPROCADDRESSPROC glad_eglGetProcAddress;
PFNEGLDESTROYCONTEXTPROC glad_eglDestroyContext;
PFNEGLSWAPINTERVALPROC glad_eglSwapInterval;
PFNEGLCOPYBUFFERSPROC glad_eglCopyBuffers;
PFNEGLRELEASETHREADPROC glad_eglReleaseThread;
PFNEGLSURFACEATTRIBPROC glad_eglSurfaceAttrib;
PFNEGLCREATEPLATFORMPIXMAPSURFACEPROC glad_eglCreatePlatformPixmapSurface;
PFNEGLGETPLATFORMDISPLAYPROC glad_eglGetPlatformDisplay;
PFNEGLCREATESYNCPROC glad_eglCreateSync;
PFNEGLGETCURRENTSURFACEPROC glad_eglGetCurrentSurface;
PFNEGLBINDAPIPROC glad_eglBindAPI;
PFNEGLCLIENTWAITSYNCPROC glad_eglClientWaitSync;
PFNEGLGETCONFIGATTRIBPROC glad_eglGetConfigAttrib;
PFNEGLGETERRORPROC glad_eglGetError;
PFNEGLCREATEPLATFORMWINDOWSURFACEPROC glad_eglCreatePlatformWindowSurface;
PFNEGLWAITNATIVEPROC glad_eglWaitNative;
PFNEGLGETDISPLAYPROC glad_eglGetDisplay;
PFNEGLQUERYSURFACEPROC glad_eglQuerySurface;
PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC glad_eglCreatePbufferFromClientBuffer;
PFNEGLWAITGLPROC glad_eglWaitGL;
PFNEGLDESTROYIMAGEPROC glad_eglDestroyImage;
PFNEGLCHOOSECONFIGPROC glad_eglChooseConfig;

static void load_EGL_VERSION_1_0(GLADloadproc load) {
	glad_eglChooseConfig = (PFNEGLCHOOSECONFIGPROC)load("eglChooseConfig");
	glad_eglCopyBuffers = (PFNEGLCOPYBUFFERSPROC)load("eglCopyBuffers");
	glad_eglCreateContext = (PFNEGLCREATECONTEXTPROC)load("eglCreateContext");
	glad_eglCreatePbufferSurface = (PFNEGLCREATEPBUFFERSURFACEPROC)load("eglCreatePbufferSurface");
	glad_eglCreatePixmapSurface = (PFNEGLCREATEPIXMAPSURFACEPROC)load("eglCreatePixmapSurface");
	glad_eglCreateWindowSurface = (PFNEGLCREATEWINDOWSURFACEPROC)load("eglCreateWindowSurface");
	glad_eglDestroyContext = (PFNEGLDESTROYCONTEXTPROC)load("eglDestroyContext");
	glad_eglDestroySurface = (PFNEGLDESTROYSURFACEPROC)load("eglDestroySurface");
	glad_eglGetConfigAttrib = (PFNEGLGETCONFIGATTRIBPROC)load("eglGetConfigAttrib");
	glad_eglGetConfigs = (PFNEGLGETCONFIGSPROC)load("eglGetConfigs");
	glad_eglGetCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC)load("eglGetCurrentDisplay");
	glad_eglGetCurrentSurface = (PFNEGLGETCURRENTSURFACEPROC)load("eglGetCurrentSurface");
	glad_eglGetDisplay = (PFNEGLGETDISPLAYPROC)load("eglGetDisplay");
	glad_eglGetError = (PFNEGLGETERRORPROC)load("eglGetError");
	glad_eglGetProcAddress = (PFNEGLGETPROCADDRESSPROC)load("eglGetProcAddress");
	glad_eglInitialize = (PFNEGLINITIALIZEPROC)load("eglInitialize");
	glad_eglMakeCurrent = (PFNEGLMAKECURRENTPROC)load("eglMakeCurrent");
	glad_eglQueryContext = (PFNEGLQUERYCONTEXTPROC)load("eglQueryContext");
	glad_eglQueryString = (PFNEGLQUERYSTRINGPROC)load("eglQueryString");
	glad_eglQuerySurface = (PFNEGLQUERYSURFACEPROC)load("eglQuerySurface");
	glad_eglSwapBuffers = (PFNEGLSWAPBUFFERSPROC)load("eglSwapBuffers");
	glad_eglTerminate = (PFNEGLTERMINATEPROC)load("eglTerminate");
	glad_eglWaitGL = (PFNEGLWAITGLPROC)load("eglWaitGL");
	glad_eglWaitNative = (PFNEGLWAITNATIVEPROC)load("eglWaitNative");
}
static void load_EGL_VERSION_1_1(GLADloadproc load) {
	glad_eglBindTexImage = (PFNEGLBINDTEXIMAGEPROC)load("eglBindTexImage");
	glad_eglReleaseTexImage = (PFNEGLRELEASETEXIMAGEPROC)load("eglReleaseTexImage");
	glad_eglSurfaceAttrib = (PFNEGLSURFACEATTRIBPROC)load("eglSurfaceAttrib");
	glad_eglSwapInterval = (PFNEGLSWAPINTERVALPROC)load("eglSwapInterval");
}
static void load_EGL_VERSION_1_2(GLADloadproc load) {
	glad_eglBindAPI = (PFNEGLBINDAPIPROC)load("eglBindAPI");
	glad_eglQueryAPI = (PFNEGLQUERYAPIPROC)load("eglQueryAPI");
	glad_eglCreatePbufferFromClientBuffer = (PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC)load("eglCreatePbufferFromClientBuffer");
	glad_eglReleaseThread = (PFNEGLRELEASETHREADPROC)load("eglReleaseThread");
	glad_eglWaitClient = (PFNEGLWAITCLIENTPROC)load("eglWaitClient");
}
static void load_EGL_VERSION_1_4(GLADloadproc load) {
	glad_eglGetCurrentContext = (PFNEGLGETCURRENTCONTEXTPROC)load("eglGetCurrentContext");
}
static void load_EGL_VERSION_1_5(GLADloadproc load) {
	glad_eglCreateSync = (PFNEGLCREATESYNCPROC)load("eglCreateSync");
	glad_eglDestroySync = (PFNEGLDESTROYSYNCPROC)load("eglDestroySync");
	glad_eglClientWaitSync = (PFNEGLCLIENTWAITSYNCPROC)load("eglClientWaitSync");
	glad_eglGetSyncAttrib = (PFNEGLGETSYNCATTRIBPROC)load("eglGetSyncAttrib");
	glad_eglCreateImage = (PFNEGLCREATEIMAGEPROC)load("eglCreateImage");
	glad_eglDestroyImage = (PFNEGLDESTROYIMAGEPROC)load("eglDestroyImage");
	glad_eglGetPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYPROC)load("eglGetPlatformDisplay");
	glad_eglCreatePlatformWindowSurface = (PFNEGLCREATEPLATFORMWINDOWSURFACEPROC)load("eglCreatePlatformWindowSurface");
	glad_eglCreatePlatformPixmapSurface = (PFNEGLCREATEPLATFORMPIXMAPSURFACEPROC)load("eglCreatePlatformPixmapSurface");
	glad_eglWaitSync = (PFNEGLWAITSYNCPROC)load("eglWaitSync");
}
static int find_extensionsEGL(void) {
	return 1;
}

int gladLoadEGLLoader(GLADloadproc load) {
	load_EGL_VERSION_1_0(load);
	load_EGL_VERSION_1_1(load);
	load_EGL_VERSION_1_2(load);
	load_EGL_VERSION_1_4(load);
	load_EGL_VERSION_1_5(load);

	if(!find_extensionsEGL()) return 0;
	return 1;
}
