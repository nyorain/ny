# Find nytl
#
# NYTIL_INCLUDE_DIRS
# NYTL_DEFINITIONS
# NYTL_FOUND

find_package(PkgConfig)
pkg_check_modules(PC_NYTL QUIET nytl)

find_path(NYTL_INCLUDE_DIR nytl/nytl.hpp
		HINTS ${PC_NYTL_INCLUDEDIR} ${PC_NYTL_INCLUDE_DIRS})

set(NYTL_DEFINITIONS ${PC_NYTL_CFLAGS_OTHER})
set(NYTL_INCLUDE_DIRS ${NYTL_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NYTL DEFAULT_MSG NYTL_INCLUDE_DIRS)
mark_as_advanced(NYTL_INCLUDE_DIR)
