include("apk.cmake" REQUIRED)

set(ANDROID_API_LEVEL 24)
set(ARM_TARGET armeabi)
set(ANDROID_APK_RUN 1)

add_library(ny-android SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../basic.cpp")
target_link_libraries(ny-android ny)
set(na-libs
	# "${CMAKE_BINARY_DIR}/src/ny/libny.so"
	"${CMAKE_BINARY_DIR}/src/examples/android/libny-android.so")
android_create_apk(ny-android "${CMAKE_BINARY_DIR}/apk" "${na-libs}" "" "")
