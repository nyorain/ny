# Copyright (c) 2017 nyorain
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

# Cmake script that uses the android sdk to create a apk from native sources.
# Needs some information about the used android api, abi and required libraries.

# requires:
# building: android, ant
# installing/running: adb, connected debug device

# used variables:
# apk_api (number for the sdk version used e.g. 24 for android 7.0)
# apk_name (the name of the apk, will create target ${apk_name}-apk)
# apk_abi (the build target of the libraries, something as "armeabi")
# apk_libraries (list of library file paths to load, must be compiled for APK_TARGET)
# apk_dir (the dir to build the apk. Is expected to have an AndroidManifest.xml)

macro(create_apk)
	# build
	add_custom_target(${apk_name}-apk ALL)

	add_custom_command(TARGET ${apk_name}-apk
		COMMAND ${CMAKE_COMMAND} -E make_directory
			"${apk_dir}/libs/${apk_abi}")

	foreach(value ${apk_libs})
		add_custom_command(TARGET ${apk_name}-apk
			COMMAND ${CMAKE_COMMAND} -E copy
				"${value}"
				"${apk_dir}/libs/${apk_abi}")
	endforeach()

	add_custom_command(TARGET ${apk_name}-apk
		COMMAND android update project
			-t android-${apk_api}
			--name ${apk_name}
			--path ${apk_dir})

	add_custom_command(TARGET ${apk_name}-apk
		COMMAND ant debug
	WORKING_DIRECTORY ${apk_dir})

	# install
	add_custom_target(${apk_name}-apk-install
		DEPENDS ${apk_name}-apk)

	add_custom_command(TARGET ${apk_name}-apk-install
		COMMAND adb install -r "${apk_dir}/bin/${apk_name}-debug.apk")
endmacro()
