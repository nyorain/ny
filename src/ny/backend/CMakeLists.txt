set(include-backend ${include-ny})

set(src-backend
		backend.cpp)

if(NY_WithWinapi)
endif(NY_WithWinapi)

if(NY_WithX11)
endif(NY_WithX11)

if(NY_WithWayland)
endif(NY_WithWayland)

add_library(ny-backend SHARED ${src-backend})
target_compile_features(ny-backend PRIVATE cxx_range_for)

target_link_libraries(ny-backend ${libs-backend})
target_include_directories(ny-backend PUBLIC ${include-backend})

set(targets-backend ny-backend)

#install
install (TARGETS ${targets-backend}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
