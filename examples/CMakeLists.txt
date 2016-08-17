#copy runtime files
add_custom_target(copyRuntimeFiles
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_BINARY_DIR}/external/install/bin/."
		"${CMAKE_BINARY_DIR}/bin"
	COMMENT "Copying binary files")

# add_executable(examples-start start/main.cpp start/nanovg.c)
# target_link_libraries(examples-start ny-draw ny-app ny-backend ny-window ny-gui)

#intro-backend
add_executable(intro-backend intro-backend/main.cpp)
target_link_libraries(intro-backend ny-base ny-backend)
add_dependencies(intro-backend copyRuntimeFiles)

#intro-app
add_executable(intro-app intro-app/main.cpp)
target_link_libraries(intro-app ny-base ny-backend ny-app)
add_dependencies(intro-app copyRuntimeFiles)
