find_package(nytl REQUIRED)
include_directories(${nytl_INCLUDE_DIRS})

add_subdirectory(base)
add_subdirectory(draw)
add_subdirectory(app)
add_subdirectory(gui)
add_subdirectory(window)
add_subdirectory(backend)
