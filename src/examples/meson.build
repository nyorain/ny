if android
	basic_example = shared_module('ny_basic', 'basic.cpp', dependencies: ny_dep)
	shared_module('ny_gl', 'gl.cpp', dependencies: ny_dep)
	# subdir('android')
else
	executable('dev', 'dev.cpp', dependencies: ny_dep)
	executable('basic', 'basic.cpp', dependencies: ny_dep)
	executable('intro', 'intro.cpp', dependencies: ny_dep)
	executable('gl', 'gl.cpp', dependencies: [ny_dep, dep_gl])

	dep_cairo = dependency('cairo', required: false)
	if dep_cairo.found()
		executable('cairo', 'cairo.cpp', dependencies: [ny_dep, dep_cairo])
	endif

	# TODO: blend2d currently has no pkg-config, just try to find the library
	lib_blend2d = meson.get_compiler('cpp').find_library('blend2d')
	if lib_blend2d.found()
		executable('b2d', 'b2d.cpp', dependencies: [ny_dep, lib_blend2d])
	endif

	executable('playground', 'playground.cpp', dependencies: [ny_dep, dep_gl])
endif
