# intro
add_executable(ny-intro intro.cpp)
target_link_libraries(ny-intro ny)

#multithread
# add_executable(ny-multithread multithread.cpp)
# target_link_libraries(ny-multithread ny pthread)

#surface
# add_executable(ny-surface surface.cpp)
# target_link_libraries(ny-surface ny)

#callbacks
# add_executable(ny-callbacks callbacks.cpp)
# target_link_libraries(ny-callbacks ny)

#vulkan
if(WithVulkan)
	# add_executable(ny-vulkan vulkan.cpp)
	# target_include_directories(ny-vulkan PUBLIC ${Vulkan_INCLUDE_DIR})
	# target_link_libraries(ny-vulkan ny ${Vulkan_LIBRARY})
endif()

#gl
if(WithGl)
	add_executable(ny-gl gl.cpp)
	target_link_libraries(ny-gl ny)
endif()

#dev
add_executable(ny-dev dev.cpp)
target_link_libraries(ny-dev ny)

#egl
add_executable(ny-egl egl.cpp)
target_link_libraries(ny-egl ny)

#inline
add_executable(ny-inline inline.cpp)
target_link_libraries(ny-inline ny)

# The following examples are currently disabled since ny does not have cairo or
# skia support anymore.
# In future they will/should be used to display how to manually integrate ny with skia
# or cairo

#cairo
# if(WithCairo)
# 	add_executable(ny-cairo cairo.cpp)
# 	target_link_libraries(ny-cairo ny ${CAIRO_LIBRARIES})
# 	target_include_directories(ny-cairo PUBLIC ${CAIRO_INCLUDE_DIRS})
#
# 	add_executable(ny-surface-cairo surface-cairo.cpp)
# 	target_link_libraries(ny-surface-cairo ny ${CAIRO_LIBRARIES})
# 	target_include_directories(ny-surface-cairo PUBLIC ${CAIRO_INCLUDE_DIRS})
# endif()


#skia
# find_package(Skia)
# if(Skia_FOUND)
# 	add_executable(ny-skia skia.cpp)
# 	target_link_libraries(ny-skia ny ${Skia_LIBRARIES} fontconfig freetype pthread)
# 	target_include_directories(ny-skia PUBLIC ${Skia_INCLUDE_DIRS})
# endif()
