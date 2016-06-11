set(include-base ${include-ny})

set(src-base
	log.cpp
	image.cpp
	cursor.cpp
	pugixml.cpp
	cursor.cpp)

add_library(ny-base SHARED ${src-base})
target_link_libraries(ny-base ${libs-base})
target_include_directories(ny-base PUBLIC ${include-base})

if(${own_nytl_build})
	add_dependencies(ny-base nytl)
endif()

set(targets-base ny-base)

#install
install (TARGETS ${targets-base}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
