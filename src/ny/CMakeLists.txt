#find_package(NYTL REQUIRED)
include_directories($ENV{NYTL_DIR})

set(src-draw
		draw/drawContext.cpp
		draw/shape.cpp
		draw/color.cpp
		draw/gradient.cpp
		draw/brush.cpp
		draw/pen.cpp
		draw/image.cpp
		draw/font.cpp)

add_library(ny-draw SHARED ${src-draw})
target_compile_features(ny-draw PRIVATE cxx_range_for)

target_link_libraries(ny-draw ${libs-draw})

install (TARGETS ny-draw
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
