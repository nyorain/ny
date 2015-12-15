#find_package(NYTL REQUIRED)
include_directories($ENV{NYTL_DIR})

set(src-draw
		drawContext.cpp
		shape.cpp
		color.cpp
		gradient.cpp
		brush.cpp
		pen.cpp
		image.cpp
		font.cpp
		triangulate.cpp)

if(NY_WithGL OR NY_WithGLES)
	set(src-draw ${src-draw} 
		gl/glContext.cpp 
		gl/glDrawContext.cpp 
		gl/glTexture.cpp
		gl/shader.cpp
		gl/drawImplementation.cpp)
	set(libs-draw ${libs-draw} glpbinding glbinding glesbinding)
endif(NY_WithGL OR NY_WithGLES)

if(NY_WithEGL)
	set(src-draw ${src-draw} gl/egl.cpp)
	set(libs-draw ${libs-draw} EGL)
endif(NY_WithEGL)

add_library(ny-draw SHARED ${src-draw})
target_compile_features(ny-draw PRIVATE cxx_range_for)

target_link_libraries(ny-draw ${libs-draw})
set(targets-draw ny-draw)

#install
install (TARGETS ${targets-draw}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)

