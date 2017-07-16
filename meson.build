project('wlroots', 'c',
    license: 'BSL',
		version: '0.1.0',
    default_options: ['cpp_std=c++1z', 'c_std=c11'])

# options
enable_x11 = get_option('enable_x11')
enable_wayland = get_option('enable_wayland')
enable_winapi = get_option('enable_winapi')
enable_vulkan = get_option('enable_vulkan')
enable_gl = get_option('enable_gl')
examples = get_option('examples')

# default arrguments
# warnings and stuff
add_project_arguments(
	'-Wall',
	'-Wextra',
	'-Wno-unused-parameter',
	'-Wpedantic',
	'-Wno-missing-braces',
	language: ['cpp', 'c'])

# project-specific stuff
add_project_arguments(
	'-DDLG_BASE_PATH="@0@"'.format(meson.source_root()),
	language: ['c', 'cpp'])

# deps
req_xkbcommon = enable_x11 or enable_wayland
req_egl = enable_wayland and enable_gl

dep_threads = dependency('threads')
dep_xkbcommon = dependency('xkbcommon', required: req_xkbcommon)
dep_vulkan = meson.get_compiler('c').find_library('vulkan', required: enable_vulkan)
dep_gl = dependency('gl', required: enable_gl and (enable_x11 or enable_winapi))
dep_egl = dependency('egl', required: req_egl)

dep_dlg = dependency('dlg', required: false)
if not dep_dlg.found()
	dlg_subproj = subproject('dlg')
	dep_dlg = dlg_subproj.get_variable('dlg_dep')
endif

dep_nytl = dependency('nytl', fallback: ['nytl', 'nytl_dep'])

deps = [dep_threads, dep_xkbcommon, dep_vulkan, dep_gl, dep_egl, dep_dlg, dep_nytl]

if enable_x11
	dep_x11 = dependency('x11')
	dep_xcursor = dependency('xcursor')
	dep_xcb = dependency('xcb')
	dep_xcbewmh = dependency('xcb-ewmh')
	dep_xcbicccm = dependency('xcb-icccm')
	deps += [dep_x11, dep_xcb, dep_xcbewmh, dep_xcbicccm]
endif

if enable_wayland
	dep_wl = dependency('wayland-client')
	dep_wlcursor = dependency('wayland-cursor')
	dep_wlegl = dependency('wayland-egl', required: enable_gl)
	deps += [dep_wl, dep_wlcursor, dep_wlegl]
endif

if enable_winapi
endif

ny_inc = include_directories('include')
ny_inc_private = include_directories('src')

# subdirs
subdir('include/ny')
subdir('src/ny')

# TODO: not working yet
if examples
	subdir('src/examples')
endif

# dependency
ny_dep = declare_dependency(
	include_directories: ny_inc,
	depdencies: deps,
	link_with: ny_lib)
