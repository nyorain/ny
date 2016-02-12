set(include-gui ${include-ny})
set(libs-gui ${libs-window} ny-base ny-app ny-backend ny-draw)

set(src-gui
	gui.cpp
	widget.cpp
	widgetBase.cpp
	button.cpp
	style.cpp)

add_library(ny-gui SHARED ${src-gui})
target_compile_features(ny-gui PRIVATE cxx_range_for)

target_link_libraries(ny-gui ${libs-gui})
target_include_directories(ny-gui PUBLIC ${include-gui})

set(targets-gui ny-gui)

#install
install (TARGETS ${targets-gui}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
