#dev
add_executable(ny-dev dev.cpp)
target_link_libraries(ny-dev ny-base ny-backend)

#intro
add_executable(ny-intro intro.cpp)
target_link_libraries(ny-intro ny-base ny-backend)

#multithread
add_executable(ny-multithread multithread.cpp)
target_link_libraries(ny-multithread ny-base ny-backend)

#surface
add_executable(ny-surface surface.cpp)
target_link_libraries(ny-surface ny-base ny-backend)

#vulkan
if(WithVulkan)
	add_executable(ny-vulkan vulkan.cpp)
	target_include_directories(ny-vulkan PUBLIC ${Vulkan_INCLUDE_DIR})
	target_link_libraries(ny-vulkan ny-base ny-backend ${Vulkan_LIBRARY})
endif()

#cairo
if(WithCairo)
	add_executable(ny-cairo cairo.cpp)
	target_link_libraries(ny-cairo ny-base ny-backend ${CAIRO_LIBRARIES})
	target_include_directories(ny-cairo PUBLIC ${CAIRO_INCLUDE_DIRS})

	add_executable(ny-surface-cairo surface-cairo.cpp)
	target_link_libraries(ny-surface-cairo ny-base ny-backend ${CAIRO_LIBRARIES})
	target_include_directories(ny-surface-cairo PUBLIC ${CAIRO_INCLUDE_DIRS})
endif()

#gl
if(WithGL)
	add_executable(ny-gl gl.cpp)
	target_link_libraries(ny-gl ny-base ny-backend)
endif()
