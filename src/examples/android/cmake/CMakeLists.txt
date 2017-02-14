add_library(ny-basic SHARED gl.cpp)
target_link_libraries(ny-basic ny GLESv2)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/cmake/android")
include("apk" REQUIRED)

# we know ANDROID_ABI from the android toolchain file
# set(apk_api ${ANDROID_PLATFORM})
set(apk_name ny-basic)
set(apk_domain com.example.ny.basic)
set(apk_libs
	"${CMAKE_CURRENT_BINARY_DIR}/libny-basic.so"
	"${CMAKE_CURRENT_BINARY_DIR}/../../../ny/libny.so")

create_apk()
add_dependencies(ny-basic-apk ny-basic)

# copy the AndroidManifest file
# add_custom_command(TARGET ny-basic-apk
# 	PRE_BUILD
# 	COMMAND ${CMAKE_COMMAND} -E copy
# 		"${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
# 		"${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml")
#
# add_custom_command(TARGET ny-basic-apk
# 	PRE_BUILD
# 	COMMAND ${CMAKE_COMMAND} -E copy_directory
# 		"${CMAKE_CURRENT_SOURCE_DIR}/src"
# 		"${CMAKE_CURRENT_BINARY_DIR}/src")
