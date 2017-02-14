# if you are interested in the building system behind the ny android apk
# example check out apk.cmake in <ny-root>/src/cmake/android
# Can be easily used to build custom android apps just using cmake
if(Android)
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/cmake/android")
	include("apk" REQUIRED)
endif()

# easy macro for example creation
# will create apk if building for android
function(create_example name libs)
	if(Android)
		add_library(ny-${name} SHARED "${name}.cpp")
		set(apk_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}/apk")
		set(load_libs
			"${CMAKE_CURRENT_BINARY_DIR}/libny-${name}.so"
			"${CMAKE_CURRENT_BINARY_DIR}/../ny/libny.so")

		create_apk("ny-${name}" "com.example.ny.${name}" "${load_libs}")
		add_dependencies(ny-${name}-apk ny-${name})
	else()
		add_executable(ny-${name} "${name}.cpp")
	endif()

	target_link_libraries(ny-${name} ny ${libs})
endfunction()

# examples
create_example(intro "")
create_example(basic "")
create_example(dev "")

# gl
if(Android)
	create_example(gl GLESv2)
else()
	if(OPENGL_FOUND)
		create_example(gl ${OPENGL_gl_LIBRARY})
	endif()
endif()
