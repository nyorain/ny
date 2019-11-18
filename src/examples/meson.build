if android
	shared_module('ny_basic', 'basic.cpp', dependencies: ny_dep)
	shared_module('ny_gl', 'gl.cpp', dependencies: ny_dep)
	# subdir('android')
else
	executable('dev', 'dev.cpp', dependencies: ny_dep)
	executable('basic', 'basic.cpp', dependencies: ny_dep)
	executable('intro', 'intro.cpp', dependencies: ny_dep)
	executable('gl', 'gl.cpp', dependencies: ny_dep)
endif
