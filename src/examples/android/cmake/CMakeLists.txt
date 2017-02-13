add_library(ny-basic SHARED gl.cpp)
target_link_libraries(ny-basic ny GLESv2)

# we know ANDROID_ABI from the android toolchain file
string(REGEX REPLACE "^android-" "" apk_api ${ANDROID_PLATFORM})
# set(apk_api ${ANDROID_PLATFORM})
set(apk_abi ${ANDROID_ABI})
set(apk_name ny-basic)
set(apk_dir ${CMAKE_CURRENT_BINARY_DIR})
set(apk_libs
	"$<TARGET_FILE:ny-basic>"
	"$<TARGET_FILE:ny>")

include("apk.cmake" REQUIRED)

create_apk()
add_dependencies(ny-basic-apk ny-basic)

# copy the AndroidManifest file
add_custom_command(TARGET ny-basic-apk
	PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
		"${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml")

add_custom_command(TARGET ny-basic-apk
	PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_BINARY_DIR}/src")
