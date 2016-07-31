set(include_app ${default_include})
set(libs_app ${default_libs} evg ny-base ny-backend)
set(src_app
		app.cpp
		keyboard.cpp
		mouse.cpp
		window.cpp
		toplevel.cpp
		child.cpp
		dialog.cpp
		popup.cpp)

add_library(ny-app SHARED ${src_app})
target_link_libraries(ny-app ${libs_app})
target_include_directories(ny-app PUBLIC ${include_app})

if(nytl_own)
	add_dependencies(ny-app nytl_ep)
endif()

if(evg_own)
	add_dependencies(ny-app evg_ep)
endif()

#install
install (TARGETS ny-app
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
