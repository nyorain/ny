add_executable(examples-start start/main.cpp)

target_link_libraries(examples-start ny-draw ny-app ny-backend ny-window ny-gui)

find_package(nytl REQUIRED)
include_directories(${nytl_INCLUDE_DIRS})
