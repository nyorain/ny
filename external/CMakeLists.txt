# function for external projects
include(ExternalProject)
set(ExternalProjectCMakeArgs
	-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/install
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

function(external_project name condition always)
	if(condition)
		ExternalProject_Add(${name}_ep
			PREFIX ${CMAKE_CURRENT_BINARY_DIR}/nytl
  	  		GIT_REPOSITORY https://github.com/nyorain/nytl.git
  			INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install/nytl
			CMAKE_ARGS ${ExternalProjectCMakeArgs}
			BUILD_ALWAYS 0)
	endif()
endfunction()

# external projects
external_project(nytl nytl_own false)

#install them
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install/." DESTINATION .)
