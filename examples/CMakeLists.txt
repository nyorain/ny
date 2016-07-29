add_executable(examples-start start/main.cpp start/nanovg.c)
target_link_libraries(examples-start ny-draw ny-app ny-backend ny-window ny-gui)
