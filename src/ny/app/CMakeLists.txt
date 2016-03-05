set(include-app ${include-ny})
set(libs-app ny-base ny-backend)

set(src-app
		app.cpp
		data.cpp
		keyboard.cpp
		mouse.cpp
		eventDispatcher.cpp)

add_library(ny-app SHARED ${src-app})
target_compile_features(ny-app PRIVATE cxx_range_for)

target_link_libraries(ny-app ${libs-app})
target_include_directories(ny-app PUBLIC ${include-app})

set(targets-app ny-app)

#install
install (TARGETS ${targets-app}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
