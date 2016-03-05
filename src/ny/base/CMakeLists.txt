set(include-base ${include-ny})

set(src-base log.cpp)

add_library(ny-base SHARED ${src-base})
target_link_libraries(ny-base ${libs-base})
target_include_directories(ny-base PUBLIC ${include-base})

set(targets-base ny-base)

#install
install (TARGETS ${targets-base}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
