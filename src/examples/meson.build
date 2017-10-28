if android
	basic_example = shared_module('basic', 'basic.cpp', dependencies: ny_dep)
	shared_module('gl', 'gl.cpp', dependencies: ny_dep)
	# subdir('android')
else
	executable('dev', 'dev.cpp', dependencies: ny_dep)
	executable('basic', 'basic.cpp', dependencies: ny_dep)
	executable('intro', 'intro.cpp', dependencies: ny_dep)
	executable('gl', 'gl.cpp', dependencies: ny_dep)
	executable('child', 'child.cpp', dependencies: ny_dep)
endif
