project('wlroots', 'c',
    license: 'MIT',
    default_options: ['c_std=c11', 'cpp_std=c++17'])

# default arrguments
add_project_arguments('-Wall', '-Wextra', '-Wno-unused-parameter', '-Werror', language: ['c', 'cpp'])

# options
enable_x11 = get_option('enable_x11')
enable_wayland = get_option('enable_wayland')
enable_winapi = get_option('enable_winapi')
enable_vulkan = get_option('enable_vulkan')
enable_gl = get_option('enable_gl')
enable_egl = get_option('enable_egl')

# deps
dep_threads = dependency('threads')
dep_xkbcommon = dependency('xkbcommon', required: enable_x11 or enable_wayland)
dep_vulkan = dependency('vulkan', required: enable_vulkan)
dep_gl = dependency('gl', required: enable_gl and (enable_x11 or enable_winapi))
dep_egl = dependency('egl', required: enable_gl and enable_wayland)

if enable_x11
	dep_x11 = dependency('x11')
	dep_xcursor = dependency('xcursor')
	dep_xcb = dependency('xcb')
	dep_xcbewmh = dependency('xcb-ewmh')
	dep_xcbicccm = dependency('xcb-icccm')
endif

if enable_wayland
	dep_wl = dependency('wayland-client')
	dep_wlcursor = dependency('wayland-cursor')
	dep_wlegl = dependency('wayland-egl', required: enable_gl)
endif

if enable_win32
endif

# subdirs
subdir('include')
subdir('src/ny')

if enable_examples
	subdir('src/examples')
endif

add_project_arguments('-DWLR_SRC_DIR="@0@"'.format(meson.source_root()), language: 'c')
add_project_arguments('-I@0@/include'.format(meson.source_root()), language: 'c')

cc = meson.get_compiler('cxx')

# Clang complains about some zeroed initialiser lists (= {0}), even though they are valid
if cc.get_id() == 'clang'
    add_project_arguments('-Wno-missing-field-initializers', language: 'c')
    add_project_arguments('-Wno-missing-braces', language: 'c')
endif


lib_wlr = library('wlroots', wlr_src,
    dependencies: all_deps)
dep_wlr = declare_dependency(link_with: lib_wlr,
    dependencies: all_deps)

lib_shared = static_library('shared',
    ['examples/shared.c', 'examples/cat.c'],
    dependencies: dep_wlr)

all_libs = [
    lib_shared,
    lib_wlr,
]

executable('simple', 'examples/simple.c', link_with: all_libs)
executable('rotation', 'examples/rotation.c', link_with: all_libs)
executable('pointer', 'examples/pointer.c', link_with: all_libs)
executable('touch', 'examples/touch.c', link_with: all_libs)
executable('tablet', 'examples/tablet.c', link_with: all_libs)
