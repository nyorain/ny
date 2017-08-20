project('ny', ['c', 'cpp'],
  license: 'BSL',
  version: '0.1.0',
  meson_version: '>=0.42',
  default_options: ['cpp_std=c++1z', 'c_std=c11'])

# options
# can be 'true', 'false' or 'auto'
# if an options is 'auto' it will be set depending
# on whether the required dpendencies are avilable
op_enable_x11 = get_option('enable_x11')
op_enable_wayland = get_option('enable_wayland')
op_enable_winapi = get_option('enable_winapi')
op_enable_vulkan = get_option('enable_vulkan')
op_enable_gl = get_option('enable_gl')
examples = get_option('examples')

# default arrguments
# warnings and stuff
add_project_arguments(
	'-Wall',
	'-Wextra',
	'-Wno-non-virtual-dtor',
	'-Wpedantic',
	'-Wno-missing-braces', # needed for clang
	language: 'cpp')

# project-specific stuff
# currently set dlg path only on linux since on windows
# it causes errors with backslashes
if build_machine.system() != 'windows'
	add_project_arguments(
		'-DDLG_BASE_PATH="@0@"'.format(meson.source_root()),
		language: 'cpp')
endif

# auto option resolution
# winapi
if op_enable_winapi == 'auto'
	enable_winapi = build_machine.system() == 'windows'
else
	enable_winapi = op_enable_winapi == 'true'
endif

# deps
dep_threads = dependency('threads')
dep_vulkan = dependency('vulkan', required: enable_vulkan)
dep_xkbcommon = dependency('xkbcommon', required: req_xkbcommon)
dep_gl = dependency('gl', required: enable_gl and (enable_x11 or enable_winapi))
dep_egl = dependency('egl', required: req_egl)

dep_dlg = dependency('dlg', fallback: ['dlg', 'dlg_dep'])
dep_nytl = dependency('nytl', fallback: ['nytl', 'nytl_dep'])

deps = [
	dep_threads,
	dep_xkbcommon,
	dep_vulkan,
	dep_gl,
	dep_egl,
	dep_dlg,
	dep_nytl]

if enable_x11
	dep_x11 = dependency('x11')
	dep_x11xcb = dependency('x11-xcb')
	dep_xcursor = dependency('xcursor')
	dep_xcb = dependency('xcb')
	dep_xcbewmh = dependency('xcb-ewmh')
	dep_xcbicccm = dependency('xcb-icccm')
	dep_xcbshm = dependency('xcb-shm')
	dep_xcbxkb = dependency('xcb-xkb')
	dep_xkbcommonx11 = dependency('xkbcommon-x11', required: req_xkbcommon)

	deps += [
		dep_x11,
		dep_x11xcb,
		dep_xcursor,
		dep_xcb,
		dep_xcbewmh,
		dep_xcbicccm,
		dep_xcbshm,
		dep_xcbxkb,
		dep_xkbcommonx11]

	enable_x11 = true
	foreach dep : x11_deps
		if not dep.found()
			enable_x11 = false
		endif
	endforeach

	if enable_x11
		deps += x11_deps
	endif
endif

# wayland
if op_enable_wayland == 'false' or not dep_xcbcommon.found()
	enable_wayland = false
else
	required = enable_wayland == 'true'
	dep_wl = dependency('wayland-client', required: required)
	dep_wlcursor = dependency('wayland-cursor', required: required)
	wl_deps += [dep_xkbcommon, dep_wl, dep_wlcursor]

	enable_wayland = true
	foreach dep : wl_deps
		if not dep.found()
			enable_wayland = false
		endif
	endforeach

	if enable_wayland
		deps += wl_deps
	endif
endif

# winapi
if enable_winapi
	deps += meson.get_compiler('cpp').find_library('Dwmapi')
	deps += meson.get_compiler('cpp').find_library('Shlwapi')
endif

if not enable_winapi and not enable_wayland and not enable_x11
	message('WARNING: building ny without any valid backends')
endif

# gl
if op_enable_gl == 'false'
	enable_gl = false
else
	required = op_enable_gl == 'true'
	if enable_winapi or enable_x11
		dep_gl = dependency('gl', required: required)
		enable_gl = dep_gl.found()
		deps += dep_gl
	endif
	if enable_wayland
		dep_egl = dependency('egl', required: required)
		dep_wlegl += dependency('wayland-egl', required: required)
		enable_gl = dep_egl.found() and dep_wlegl.found()
		deps += dep_gl
	endif
endif

# vulkan TODO
if op_enable_vulkan == 'false'
	enable_vulkan = false
else
	required = op_enable_vulkan == 'true'
	dep_vulkan = dependency('vulkan', required)
	enable_vulkan = dep_vulkan.found()
	deps += dep_vulkan
endif

dep_dlg = dependency('dlg', fallback: ['dlg', 'dlg_dep'])
dep_nytl = dependency('nytl', fallback: ['nytl', 'nytl_dep'])

# common dependencies
deps += [
	dep_threads,
	dep_gl,
	dep_egl,
	dep_dlg,
	dep_nytl]

ny_inc = include_directories('include')
ny_inc_private = include_directories('src')

# subdirs
subdir('include/ny')
subdir('src/ny')

# dependency
message(ny_lib_dir)
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
