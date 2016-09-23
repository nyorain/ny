#copy runtime files
add_custom_target(copyRuntimeFiles
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_BINARY_DIR}/external/install/bin/."
		"${CMAKE_BINARY_DIR}/bin"
	COMMENT "Copying binary files")

# add_executable(examples-start start/main.cpp start/nanovg.c)
# target_link_libraries(examples-start ny-draw ny-app ny-backend ny-window ny-gui)

#dev
# add_executable(ny-dev dev/main.cpp)
add_executable(ny-dev dev/main2.cpp)
target_link_libraries(ny-dev ny-base ny-backend cairo)
add_dependencies(ny-dev copyRuntimeFiles)

#intro-backend
add_executable(ny-intro-backend intro-backend/main.cpp)
target_link_libraries(ny-intro-backend ny-base ny-backend)
add_dependencies(ny-intro-backend copyRuntimeFiles)

#intro-app
add_executable(ny-intro-app intro-app/main.cpp)
target_link_libraries(ny-intro-app ny-base ny-backend ny-app)
add_dependencies(ny-intro-app copyRuntimeFiles)

#vulkan
add_executable(ny-vulkan vulkan/main.cpp)
target_include_directories(ny-vulkan PUBLIC ${VULKAN_INCLUDE_DIR})
target_link_libraries(ny-vulkan ny-base ny-backend vpp ${VULKAN_LIB})
add_dependencies(ny-vulkan copyRuntimeFiles)
