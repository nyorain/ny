# Try to find vpp
# Once done this will define
# ${pkg-name}_FOUND
# ${pkg-name}_INCLUDE_DIRS
# ${pkg-name}_LIBRARIES

#config
set(pkg-name nytl)
set(pkg-libraries false)

#defaults
set(pkg-config-search ${pkg-name})
set(pkg-library-search ${pkg-name} lib${pkg-name})
set(pkg-include-search ${pkg-name}/${pkg-name}.hpp)

#fixed procedure
set(${pkg-name}-search-paths
	./lib/${pkg-name}
	$ENV{PROGRAMFILES}/${pkg-name}
	"$ENV{PROGRAMFILES\(X86\)}/${pkg-name}"
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/opt
	/sw
	/opt/local
	/opt/csw)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
	pkg_check_modules(PC_${pkg-name} QUIET ${pkg-config-search})
	set(${pkg-name}-search-paths
		${${pkg-name}-search-paths}
		${PC_${pkg-name}_INCLUDE_DIRS}
		${PC_${pkg-name}_INCLUDEDIR}
		${PC_${pkg-name}_LIBRARY_DIRS}
		${PC_${pkg-name}_LIBDIR})
endif(PkgConfig_FOUND)


find_path(${pkg-name}_INCLUDE_DIRS ${pkg-include-search}
	HINTS ${${pkg-name}-search-paths}
	PATH_SUFFIXES ${pkg-name} include)

include(FindPackageHandleStandardArgs)

if(pkg-libraries)
	find_library(${pkg-name}_LIBRARIES
		NAMES ${pkg-library-search}
		HINTS ${vpp-search-paths}
		PATH_SUFFIXES ${pkg-name} lib lib64 lib/x86 lib/x64)

	find_package_handle_standard_args(${pkg-name} DEFAULT_MSG
		${pkg-name}_LIBRARIES ${pkg-name}_INCLUDE_DIRS)
	mark_as_advanced(${pkg-name}_LIBRARIES ${pkg-name}_INCLUDE_DIRS)

else(pkg-libraries)
	find_package_handle_standard_args(${pkg-name} DEFAULT_MSG ${pkg-name}_INCLUDE_DIRS)
	mark_as_advanced(${pkg-name}_INCLUDE_DIRS)

endif(pkg-libraries)
