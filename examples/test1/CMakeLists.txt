add_executable(test1 main.cpp)

target_compile_features(test1 PRIVATE cxx_range_for)
target_link_libraries(test1 ny)

install(TARGETS test1 DESTINATION ${CMAKE_BINARY_DIR}/examplesBuilt)
