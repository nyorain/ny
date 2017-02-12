set(ANDROID_API_LEVEL 24)
set(ARM_TARGET armeabi)
set(ANDROID_NAME ny-android)

add_library(ny-android SHARED gl.cpp)
target_link_libraries(ny-android ny GLESv2)

set(ANDROID_LIBS
	"$<TARGET_FILE:ny-android>"
	"$<TARGET_FILE:ny>")

# we know ANDROID_ABI from the android toolchain file
foreach(value ${ANDROID_LIBS})
	add_custom_command(TARGET ${ANDROID_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			${value}
			"${CMAKE_CURRENT_BINARY_DIR}/libs/${ANDROID_ABI}")
endforeach()

add_custom_command(TARGET ny-android
	COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
		"${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml")

add_custom_command(TARGET ny-android
	COMMAND android update project
		-t android-${ANDROID_API_LEVEL}
		--name ${ANDROID_NAME}
		--path ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(TARGET ny-android
	COMMAND ant debug
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
