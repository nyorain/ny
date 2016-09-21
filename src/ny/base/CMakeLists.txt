set(include_base ${default_include})
set(libs_base ${default_libs} evg)
set(src_base log.cpp cursor.cpp data.cpp eventDispatcher.cpp imageData.cpp)

add_library(ny-base SHARED ${src_base})
target_link_libraries(ny-base ${libs_base})
target_include_directories(ny-base PUBLIC ${include_base})

if(evg_build)
	add_dependencies(ny-base evg_ep)
endif()

#install
install (TARGETS ny-base
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
