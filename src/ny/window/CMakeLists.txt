set(include-window ${include-ny})
set(libs-window ${libs-window} ny-base ny-app ny-backend ny-draw)

set(src-window
		window.cpp
		toplevel.cpp
		child.cpp
		dialog.cpp
		popup.cpp
		frame.cpp)

add_library(ny-window SHARED ${src-window})
target_compile_features(ny-window PRIVATE cxx_range_for)


target_link_libraries(ny-window ${libs-window})
target_include_directories(ny-window PUBLIC ${include-window})

set(targets-window ny-window)

#install
install (TARGETS ${targets-window}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
