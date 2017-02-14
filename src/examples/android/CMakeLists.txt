add_library(ny-basic SHARED gl.cpp)
target_link_libraries(ny-basic ny GLESv2)

# if you are interested in the building system behind the ny android apk
# example check out apk.cmake in <ny-root>/src/cmake/android
# Can be easily used to build custom android apps just using cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/cmake/android")
include("apk" REQUIRED)

# override some apk.cmake values
set(apk_name ny-basic)
set(apk_domain com.example.ny.basic)
set(apk_libs
	"${CMAKE_CURRENT_BINARY_DIR}/libny-basic.so"
	"${CMAKE_CURRENT_BINARY_DIR}/../../ny/libny.so")

create_apk()
add_dependencies(ny-basic-apk ny-basic)
