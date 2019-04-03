project('ny', ['c', 'cpp'],
  license: 'BSL',
  version: '0.1.0',
  meson_version: '>=0.47',
  default_options: ['cpp_std=c++1z', 'c_std=c11', 'warning_level=3'])

# TODO:
# android:
# - disable x11/wayland/winapi dependency checks, maybe error with wrong options

# options
# can be 'true', 'false' or 'auto'
# if an options is 'auto' it will be set depending
# on whether the required dpendencies are avilable
op_enable_x11 = get_option('enable_x11')
op_enable_wayland = get_option('enable_wayland')
op_enable_winapi = get_option('enable_winapi')
op_enable_vulkan = get_option('enable_vulkan')
op_enable_gl = get_option('enable_gl')
op_enable_egl = get_option('enable_egl')
examples = get_option('examples')
android = get_option('android')

# default arrguments
# warnings and stuff
add_project_arguments(
	'-Wno-missing-braces', # NOTE: only needed for clang
	language: 'cpp')

# TODO: fix on windows, see dlg notes
# project-specific stuff
# currently set dlg path only on linux since on windows
# it causes errors with backslashes
if build_machine.system() != 'windows'
	add_project_arguments(
		'-DDLG_BASE_PATH="@0@"'.format(meson.source_root()),
		language: 'cpp')
endif

# auto option resolution
# always required deps
dep_threads = dependency('threads')
dep_dlg = dependency('dlg', fallback: ['dlg', 'dlg_dep'])
dep_nytl = dependency('nytl', fallback: ['nytl', 'nytl_dep'])

deps = [
	dep_threads,
	dep_dlg,
	dep_nytl]

# vulkan
dep_vulkan = dependency('vulkan', required: op_enable_vulkan)
enable_vulkan = dep_vulkan.found()
deps += dep_vulkan

# gl
# note added to deps automatically since normally not needed
# backends will add if they need it
dep_gl = dependency('gl', required: op_enable_gl)
enable_gl = dep_gl.found()

if android
	dep_egl = meson.get_compiler('c').find_library('EGL', required: op_enable_egl)
	enable_egl = dep_egl.found()
else
	dep_egl = dependency('egl', required: op_enable_egl)
	enable_egl = dep_egl.found()
endif

# required for wayland and x11 backends though
dep_xkbcommon = dependency('xkbcommon', required:
	op_enable_x11 == 'enabled' or
	op_enable_wayland == 'enabled')

# winapi
if op_enable_winapi == 'auto'
	enable_winapi = build_machine.system() == 'windows'
else
	enable_winapi = op_enable_winapi == 'true'
endif

if enable_winapi
	deps += meson.get_compiler('cpp').find_library('Dwmapi')
	deps += meson.get_compiler('cpp').find_library('Shlwapi')
endif


# x11
x11_deps = [
	dep_xkbcommon,
	dependency('x11', required: op_enable_x11),
	dependency('x11-xcb', required: op_enable_x11),
	dependency('xi', required: op_enable_x11),
	dependency('xcursor', required: op_enable_x11),
	dependency('xcb', required: op_enable_x11),
	dependency('xcb-ewmh', required: op_enable_x11),
	dependency('xcb-icccm', required: op_enable_x11),
	dependency('xcb-shm', required: op_enable_x11),
	dependency('xcb-xkb', required: op_enable_x11),
	dependency('xkbcommon-x11', required: op_enable_x11),
]

enable_x11 = true
foreach dep : x11_deps
	if not dep.found()
		enable_x11 = false
	endif
endforeach
deps += x11_deps

if enable_x11 and enable_gl
	deps += dep_gl
endif

# wayland
dep_wl_protos = dependency('wayland-protocols', version: '>=1.17',
	required: op_enable_wayland)
wl_deps = [
	dep_xkbcommon,
	dependency('wayland-client', required: op_enable_wayland),
	dependency('wayland-cursor', required: op_enable_wayland),
	dep_wl_protos,
]

enable_wayland = true
foreach dep : wl_deps
	if not dep.found()
		enable_wayland = false
	endif
endforeach
deps += wl_deps

if enable_wayland and enable_egl
  deps += dep_egl
  deps += dependency('wayland-egl')
endif

# check that at least backend is supported
# otherwise at least output warning, since this is probably not desired
if not enable_winapi and not enable_wayland and not enable_x11 and not android
	message('WARNING: building ny without any valid backends')
endif

ny_inc = include_directories('include')
ny_inc_private = include_directories('src')

# subdirs
subdir('include/ny')
subdir('src/ny')

# dependency
ny_dep = declare_dependency(
	include_directories: ny_inc,
	dependencies: deps,
	link_with: ny_lib)

# examples
# must come after dependency
if examples
	subdir('src/examples')
endif

# pkgconfig
# TODO: make sure requires is correct (test it)
# test the packageconfig with an external project
if not android
  pkg = import('pkgconfig')
  pkg_dirs = ['ny']
  pkg.generate(
	  name: 'ny',
	  requires: ['nytl'],
	  libraries: ny_lib,
	  filebase: 'ny',
	  subdirs: pkg_dirs,
	  version: meson.project_version(),
	  description: 'C++17 window abstraction')
endif
