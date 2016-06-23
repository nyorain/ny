set(include-draw ${include-ny})
set(libs-draw ny-base)

set(src-draw
	drawContext.cpp
	shape.cpp
	color.cpp
	gradient.cpp
	brush.cpp
	pen.cpp
	svg.cpp
	font.cpp
	triangulate.cpp)

if(${WithGL})
	set(include-draw ${include-draw} "${CMAKE_SOURCE_DIR}/include/ny/draw/gl/glad")
	set(src-draw ${src-draw}
		gl/glad/glad.c
		gl/context.cpp
		gl/drawContext.cpp
		gl/texture.cpp
		gl/shader.cpp
		gl/shaderGenerator.cpp
		gl/resource.cpp)
endif()

if(${WithEGL})
	set(src-draw ${src-draw} gl/egl.cpp)
	set(libs-draw ${libs-draw} EGL)
endif()

if(${WithCairo})
	set(src-draw ${src-draw} cairo.cpp)
	set(libs-draw ${libs-draw} cairo)
endif()

if(${WithFreeType})
	#find_package(FreeType)
	set(src-draw ${src-draw} freeType.cpp)
	set(libs-draw ${libs-draw} freetype)
	#set(libs-draw ${libs-draw} ${FREETYPE_LIBRARIES})
	#set(include-draw ${include-draw} ${FREETYPE_INCLUDE_DIRS})
endif()

if(${WithWinapi})
	set(src-draw ${src-draw} gdi.cpp)
	#set(libs-draw ${libs-draw} "Gdiplus")
endif()

add_library(ny-draw SHARED ${src-draw})
target_link_libraries(ny-draw ${libs-draw})
target_include_directories(ny-draw PUBLIC ${include-draw})

if(${own_nytl_build})
	add_dependencies(ny-draw nytl_ep)
endif()

set(targets-draw ny-draw)

#install
install (TARGETS ${targets-draw}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
