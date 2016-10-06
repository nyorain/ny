#.rst:
# FindXKBCommon
# -------
#
# Find XKBCommon library
#
# Try to find XKBCommon library. The following values are defined
#
# ::
#
#   XKBCOMMON_FOUND         - True if XKBCommon is available
#   XKBCOMMON_INCLUDE_DIRS  - Include directories for XKBCommon
#   XKBCOMMON_LIBRARIES     - List of libraries for XKBCommon
#   XKBCOMMON_DEFINITIONS   - List of definitions for XKBCommon
#
#=============================================================================
# Copyright (c) 2015 Jari Vetoniemi
#
# Distributed under the OSI-approved BSD License (the "License");
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
#
# Edited by nyorain to also find xkbcommon-x11.

include(FeatureSummary)
set_package_properties(XKBCommon PROPERTIES
   URL "http://xkbcommon.org/"
   DESCRIPTION "Library to handle keyboard descriptions")

find_package(PkgConfig)
pkg_check_modules(PC_XKBCOMMON QUIET xkbcommon)
find_path(XKBCOMMON_INCLUDE_DIRS NAMES xkbcommon/xkbcommon.h HINTS ${PC_XKBCOMMON_INCLUDE_DIRS})

find_library(XKBCOMMON_LIBRARY NAMES xkbcommon HINTS ${PC_XKBCOMMON_LIBRARY_DIRS})
find_library(XKBCOMMON_LIBRARY_x11 NAMES xkbcommon-x11 HINTS ${PC_XKBCOMMON_LIBRARY_DIRS})
set(XKBCOMMON_LIBRARIES ${XKBCOMMON_LIBRARY} ${XKBCOMMON_LIBRARY_x11})

set(XKBCOMMON_DEFINITIONS ${PC_XKBCOMMON_CFLAGS_OTHER})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XKBCOMMON DEFAULT_MSG 
	XKBCOMMON_LIBRARY 
	XKBCOMMON_LIBRARY_x11
	XKBCOMMON_INCLUDE_DIRS)
mark_as_advanced(XKBCOMMON_LIBRARY XKBCOMMON_LIBRARY_x11 XKBCOMMON_INCLUDE_DIRS XKBCOMMON_DEFINITIONS)
