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

    EGL loader generated by glad 0.1.10a0 on Thu May 26 18:24:25 2016.

    Language/Generator: C/C++
    Specification: egl
    APIs: egl=1.5
    Profile: -
    Extensions:
        EGL_EXT_buffer_age, EGL_EXT_device_enumeration, EGL_EXT_platform_base, EGL_KHR_image, EGL_KHR_image_base, EGL_KHR_platform_android, EGL_KHR_platform_gbm, EGL_KHR_platform_wayland, EGL_KHR_platform_x11, EGL_KHR_surfaceless_context, EGL_KHR_swap_buffers_with_damage, EGL_MESA_drm_image
    Loader: No

    Commandline:
        --api="egl=1.5" --generator="c" --spec="egl" --no-loader --extensions="EGL_EXT_buffer_age,EGL_EXT_device_enumeration,EGL_EXT_platform_base,EGL_KHR_image,EGL_KHR_image_base,EGL_KHR_platform_android,EGL_KHR_platform_gbm,EGL_KHR_platform_wayland,EGL_KHR_platform_x11,EGL_KHR_surfaceless_context,EGL_KHR_swap_buffers_with_damage,EGL_MESA_drm_image"
    Online:
        http://glad.dav1d.de/#language=c&specification=egl&api=egl%3D1.5&extensions=EGL_EXT_buffer_age&extensions=EGL_EXT_device_enumeration&extensions=EGL_EXT_platform_base&extensions=EGL_KHR_image&extensions=EGL_KHR_image_base&extensions=EGL_KHR_platform_android&extensions=EGL_KHR_platform_gbm&extensions=EGL_KHR_platform_wayland&extensions=EGL_KHR_platform_x11&extensions=EGL_KHR_surfaceless_context&extensions=EGL_KHR_swap_buffers_with_damage&extensions=EGL_MESA_drm_image
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ny/common/eglApi.hpp>

int gladLoadEGL(void) {
    return gladLoadEGLLoader((GLADloadproc)eglGetProcAddress);
}

PFNEGLQUERYDEVICESEXTPROC glad_eglQueryDevicesEXT;
PFNEGLGETPLATFORMDISPLAYEXTPROC glad_eglGetPlatformDisplayEXT;
PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC glad_eglCreatePlatformWindowSurfaceEXT;
PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC glad_eglCreatePlatformPixmapSurfaceEXT;
PFNEGLCREATEIMAGEKHRPROC glad_eglCreateImageKHR;
PFNEGLDESTROYIMAGEKHRPROC glad_eglDestroyImageKHR;
PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC glad_eglSwapBuffersWithDamageKHR;
PFNEGLCREATEDRMIMAGEMESAPROC glad_eglCreateDRMImageMESA;
PFNEGLEXPORTDRMIMAGEMESAPROC glad_eglExportDRMImageMESA;
static void load_EGL_EXT_device_enumeration(GLADloadproc load) {
	glad_eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC)load("eglQueryDevicesEXT");
}
static void load_EGL_EXT_platform_base(GLADloadproc load) {
	glad_eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)load("eglGetPlatformDisplayEXT");
	glad_eglCreatePlatformWindowSurfaceEXT = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)load("eglCreatePlatformWindowSurfaceEXT");
	glad_eglCreatePlatformPixmapSurfaceEXT = (PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC)load("eglCreatePlatformPixmapSurfaceEXT");
}
static void load_EGL_KHR_image(GLADloadproc load) {
	glad_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)load("eglCreateImageKHR");
	glad_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)load("eglDestroyImageKHR");
}
static void load_EGL_KHR_image_base(GLADloadproc load) {
	glad_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)load("eglCreateImageKHR");
	glad_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)load("eglDestroyImageKHR");
}
static void load_EGL_KHR_swap_buffers_with_damage(GLADloadproc load) {
	glad_eglSwapBuffersWithDamageKHR = (PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC)load("eglSwapBuffersWithDamageKHR");
}
static void load_EGL_MESA_drm_image(GLADloadproc load) {
	glad_eglCreateDRMImageMESA = (PFNEGLCREATEDRMIMAGEMESAPROC)load("eglCreateDRMImageMESA");
	glad_eglExportDRMImageMESA = (PFNEGLEXPORTDRMIMAGEMESAPROC)load("eglExportDRMImageMESA");
}
static int find_extensionsEGL(void) {
	return 1;
}

static void find_coreEGL(void) {
}

int gladLoadEGLLoader(GLADloadproc load) {
	find_coreEGL();

	if (!find_extensionsEGL()) return 0;
	load_EGL_EXT_device_enumeration(load);
	load_EGL_EXT_platform_base(load);
	load_EGL_KHR_image(load);
	load_EGL_KHR_image_base(load);
	load_EGL_KHR_swap_buffers_with_damage(load);
	load_EGL_MESA_drm_image(load);
	return 1;
}
