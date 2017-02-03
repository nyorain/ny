# intro
add_executable(ny-intro intro.cpp)
target_link_libraries(ny-intro ny)

# basic
add_executable(ny-basic basic.cpp)
target_link_libraries(ny-basic ny)

# gl
if(WithGl)
	add_executable(ny-gl gl.cpp)
	target_link_libraries(ny-gl ny)
endif()

# dev
add_executable(ny-dev dev.cpp)
target_link_libraries(ny-dev ny)
