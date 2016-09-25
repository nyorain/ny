#dev
add_executable(ny-dev dev/main2.cpp)
target_link_libraries(ny-dev ny-base ny-backend)

#intro-backend
add_executable(ny-intro-backend intro-backend/main.cpp)
target_link_libraries(ny-intro-backend ny-base ny-backend)

#intro-app
add_executable(ny-intro-app intro-app/main.cpp)
target_link_libraries(ny-intro-app ny-base ny-backend ny-app)

#vulkan
#TODO: sth about the vpp dependency
if(WithVulkan)
	add_executable(ny-vulkan vulkan/main.cpp)
	target_include_directories(ny-vulkan PUBLIC ${VULKAN_INCLUDE_DIR})
	target_link_libraries(ny-vulkan ny-base ny-backend vpp ${VULKAN_LIB})
endif()

#cairo
if(WithCairo)
	add_executable(ny-cairo cairo/main.cpp)
	target_link_libraries(ny-cairo ny-base ny-backend ${CAIRO_LIBRARIES})
	target_include_directories(ny-cairo PUBLIC ${CAIRO_INCLUDE_DIRS})
endif()
