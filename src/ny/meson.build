ny_include = []
ny_defs = []
ny_libs = []

ny_src = [
	'config.cpp',
	'cursor.cpp',
	'image.cpp',
	'dataExchange.cpp',
	'key.cpp',
	'mouseButton.cpp',
	'windowListener.cpp',
	'backend.cpp',
	'common/gl.cpp', # does actually not need gl
	]

# Winapi backend
if enable_winapi
	# requires vista
    add_project_arguments('-D_WIN32_WINNT=0x0600', language: 'cpp')
	ny_src += [
		'winapi/appContext.cpp',
		'winapi/backend.cpp',
		'winapi/bufferSurface.cpp',
		'winapi/com.cpp',
		'winapi/input.cpp',
		'winapi/util.cpp',
		'winapi/windowContext.cpp']

	ny_libs += ['gdi32', 'gdiplus', 'Dwmapi', 'Shlwapi']

	if enable_gl
		ny_src += ['winapi/wgl.cpp']
	endif

	if enable_vulkan
		ny_src += ['winapi/vulkan.cpp']
	endif
endif

# X11 backend
if enable_x11
	ny_src += [
		'x11/appContext.cpp',
		'x11/backend.cpp',
		'x11/bufferSurface.cpp',
		'x11/dataExchange.cpp',
		'x11/input.cpp',
		'x11/util.cpp',
		'x11/windowContext.cpp']

	if enable_gl
		ny_src += ['x11/glx.cpp']
	endif

	if enable_vulkan
		ny_src += ['x11/vulkan.cpp']
	endif
endif

# Wayland backend
if enable_wayland
	ny_src += [
		'wayland/appContext.cpp',
		'wayland/backend.cpp',
		'wayland/bufferSurface.cpp',
		'wayland/dataExchange.cpp',
		'wayland/input.cpp',
		'wayland/util.cpp',
		'wayland/windowContext.cpp',

		'wayland/protocols/xdg-shell-v5.c',
		'wayland/protocols/xdg-shell-v6.c']

	if enable_gl
		ny_src += ['wayland/egl.cpp']
	endif

	if enable_vulkan
		ny_src += ['wayland/vulkan.cpp']
	endif
endif

# android backend
if android
	ny_src += [
		'android/appContext.cpp',
		'android/activity.cpp',
		'android/backend.cpp',
		'android/bufferSurface.cpp',
		'android/windowContext.cpp',
		'android/input.cpp',
		'android/mainProxy.c']

	deps += meson.get_compiler('c').find_library('log')
	deps += meson.get_compiler('c').find_library('android')

	if enable_egl
		ny_src += ['android/egl.cpp']
	endif

	if enable_vulkan
		ny_src += ['android/vulkan.cpp']
	endif
endif

# additional
if enable_egl
	ny_src += ['common/egl.cpp']
endif

if dep_xkbcommon.found()
	ny_src += ['common/xkb.cpp']
	deps += [dep_xkbcommon] # should probably not be here
endif

if enable_wayland or enable_x11
	ny_src += ['common/unix.cpp']
endif

if android
	add_project_arguments('-fexceptions', language: 'c')
	ny_lib = shared_module('ny',
		sources: ny_src,
		dependencies: deps,
		cpp_args: '-DDLG_DEFAULT_TAGS="ny",',
		include_directories: [ny_inc, ny_inc_private])
else
	ny_lib = library('ny',
		sources: ny_src,
		install: true,
		dependencies: deps,
		cpp_args: '-DDLG_DEFAULT_TAGS="ny",',
		include_directories: [ny_inc, ny_inc_private])
endif

ny_lib_dir = join_paths(meson.build_root(), 'src', 'ny') + '/'
