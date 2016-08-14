# function to update submodule
function(update_submodule submodule)
	execute_process(
		COMMAND git submodule update --remote --merge --init ${submodule}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

# function for external projects
include(ExternalProject)
set(ExternalProjectCMakeArgs
	-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/install
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

function(external_project name condition)
	if(condition)
		update_submodule(${name})
		ExternalProject_Add(${name}_ep
			SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
			CMAKE_ARGS ${ExternalProjectCMakeArgs}
			BUILD_ALWAYS 1)
	endif()
endfunction()

# external projects
external_project(evg build_evg)

#install them
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install/." DESTINATION .)
