# Copyright (c) 2017 nyorain
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

# Cmake script that uses the android sdk to create a apk from native sources.
# Needs some information about the used android api, abi and required libraries.

# requires:
# building: android, ant
# installing/running: adb, connected debug device

# used variables:
# apk_name (the name of the apk, will create target ${apk_name}-apk)
# apk_domain (the domain of the apk, sth like com.example.ny.basic)
# apk_libraries (list of library file paths to load, must be compiled for APK_TARGET)

# apk_api (number for the sdk version used e.g. 24 for android 7.0)
# apk_abi (the build target of the libraries, something as "armeabi")
# apk_dir (the dir to build the apk. Is expected to have an AndroidManifest.xml)
# apk_min_api (the minimal supported sdk api)
# apk_debug

# here we set the defaults. Override them
string(REGEX REPLACE "^android-" "" apk_api ${ANDROID_PLATFORM})
set(apk_abi ${ANDROID_ABI})
set(apk_dir "${CMAKE_CURRENT_BINARY_DIR}/apk")
set(apk_min_api 9)
set(apk_debug 1)

# TODO: remove lib dir in every build process?
# TODO: assets and such stuff
# TODO: strings.xml.in, don't just use ${apk_name} for name and label in androidManifest
# TODO: at some point in future the LoaderActivity.java can be removed since newer android
# 	versions fix the bug that libraries from the android xml (android.app.lib_name) are not loaded
# 	Investigate this further. Is it really needed anymore?

set(apk_current_dir ${CMAKE_CURRENT_LIST_DIR})

macro(create_apk)
	# debug - release switch
	if(apk_debug)
		set(apk_build debug)
		set(apk_debug_value true)
	else()
		set(apk_build release)
		set(apk_debug_value false)
	endif()

	# create abi libs directory
	add_custom_command(
		OUTPUT "${apk_dir}/libs/${apk_abi}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${apk_dir}/libs/${apk_abi}")

	# process libs
	foreach(lib ${apk_libs})
		# copy each file to lib dir
		get_filename_component(lib_name ${lib} NAME)
		add_custom_command(
			OUTPUT "${apk_dir}/libs/${apk_abi}/${lib_name}"
			COMMAND ${CMAKE_COMMAND} -E copy
				"${lib}"
				"${apk_dir}/libs/${apk_abi}/${lib_name}"
			DEPENDS "${value}")

		list(APPEND apk_dep_libs "${apk_dir}/libs/${apk_abi}/${lib_name}")

		# get raw lib name for java loading configure file
		get_filename_component(lib_name_load ${lib} NAME_WE)
		string(REGEX REPLACE "^lib" "" lib_name_load_plain ${lib_name_load})
		list(APPEND apk_load_libs ${lib_name_load_plain})
	endforeach()

	# warning for generated sources
	set(java_gen_warning "// Generated file, rather edit LoaderActivity.java.in")
	set(xml_gen_warning "<!-- Generated file, rather edit AndroidManifest.xml.in -->")

	# java config file
	# this file is needed to load all libraries
	string(REGEX REPLACE "\\\." "/" domain_dir ${apk_domain})
	configure_file(
		"${apk_current_dir}/LoaderActivity.java.in"
		"${apk_dir}/src/${domain_dir}/LoaderActivity.java")

	# android xml config file
	configure_file(
		"${apk_current_dir}/AndroidManifest.xml.in"
		"${apk_dir}/AndroidManifest.xml")

	# setup android project
	add_custom_command(
		OUTPUT "${apk_dir}/build.xml"
		DEPENDS "${apk_dir}/AndroidManifest.xml"
		COMMAND android update project
			-t android-${apk_api}
			--name ${apk_name}
			--path ${apk_dir}
		WORKING_DIRECTORY "${apk_dir}")

	# build it using ant
	add_custom_command(
		OUTPUT "${apk_dir}/bin/${apk_name}-${apk_build}.apk"
		COMMAND ant ${apk_build}
		DEPENDS
			"${apk_dir}/build.xml"
			"${apk_dir}/src/${domain_dir}/LoaderActivity.java"
			"${apk_dir}/AndroidManifest.xml"
			"${apk_dep_libs}"
		WORKING_DIRECTORY ${apk_dir})

	# build target
	# will execute the above commands
	add_custom_target(${apk_name}-apk ALL
		DEPENDS "${apk_dir}/bin/${apk_name}-${apk_build}.apk")

	# install
	# this target install the apk to a device via adb install
	add_custom_target(${apk_name}-apk-install
		DEPENDS ${apk_name}-apk)

	add_custom_command(TARGET
		${apk_name}-apk-install
		COMMAND ${CMAKE_COMMAND} -E touch "${apk_dir}/install-apk.stamp"
		COMMAND adb install -r "${apk_dir}/bin/${apk_name}-${apk_build}.apk")
endmacro()
