execute_process(
	COMMAND git submodule update --remote --merge --init
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include(ExternalProject)
set(ExternalProjectCMakeArgs
	-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/install
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

#external projects
ExternalProject_Add(nytl_ep
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/nytl"
	CMAKE_ARGS ${ExternalProjectCMakeArgs})

#install them
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/install/." DESTINATION .)
