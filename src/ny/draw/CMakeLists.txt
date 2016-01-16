set(include-draw ${include-ny})

set(src-draw
		drawContext.cpp
		shape.cpp
		color.cpp
		gradient.cpp
		brush.cpp
		pen.cpp
		image.cpp
		svg.cpp
		font.cpp
		triangulate.cpp)

if(NY_WithGL OR NY_WithGLES)
	set(src-draw ${src-draw} 
		gl/glContext.cpp 
		gl/glDrawContext.cpp 
		gl/glTexture.cpp
		gl/shader.cpp)
	set(libs-draw ${libs-draw} glpbinding glbinding glesbinding)
endif(NY_WithGL OR NY_WithGLES)

if(NY_WithEGL)
	set(src-draw ${src-draw} gl/egl.cpp)
	set(libs-draw ${libs-draw} EGL)
endif(NY_WithEGL)

if(NY_WithCairo)
	set(src-draw ${src-draw} cairo.cpp)
	set(libs-draw ${libs-draw} cairo)
endif(NY_WithCairo)

if(NY_WithFreeType)
	find_package(Freetype REQUIRED)
	set(src-draw ${src-draw} freeType.cpp)
	set(libs-draw ${libs-draw} ${FREETYPE_LIBRARIES})
	set(include-draw ${include-draw} ${FREETYPE_INCLUDE_DIRS})
endif(NY_WithFreeType)


add_library(ny-draw SHARED ${src-draw})
target_compile_features(ny-draw PRIVATE cxx_range_for)

target_link_libraries(ny-draw ${libs-draw})
target_include_directories(ny-draw PUBLIC ${include-draw})

set(targets-draw ny-draw)

#install
install (TARGETS ${targets-draw}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)

