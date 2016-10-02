#   Skia_FOUND
#   Skia_INCLUDE_DIR
#   Skia_LIBRARY

set(Skia_BUILD_DIR "" CACHE PATH "Skia build directory")

find_path(Skia_INCLUDE_DIR_base
  core/SkCanvas.h
  HINTS ${Skia_BUILD_DIR}/include)
mark_as_advanced(Skia_INCLUDE_DIR_base)

# set(skia-libs svg xml views xps codec core effects core skgpu core skgputest images ports 
# 	utils opts opts_avx opts_hsw opts_sse41 opts_sse42 opts_ssse3 pdf sfnt)
set(skia-libs core effects ports core codec skgpu core images effects core ports effects core ports images utils 
	opts opts_avx opts_hsw opts_sse41 opts_sse42 opts_ssse3 skgpu ports core sfnt xps xml svg views
	core effects ports core skgpu core images effects core ports effects core ports images utils 
	opts opts_avx opts_hsw opts_sse41 opts_sse42 opts_ssse3 skgpu ports core sfnt xps xml svg views
	skgputest svgdom sksl)
foreach(lib ${skia-libs})
	find_library(Skia_LIBRARY_${lib}
		skia_${lib}
		PATH ${Skia_BUILD_DIR}/out/Debug)
	list(APPEND Skia_LIBRARIES ${Skia_LIBRARY_${lib}})
	list(APPEND skia-lib-check Skia_LIBRARY_${lib})
	mark_as_advanced(Skia_LIBRARY_${lib})
endforeach()

set(dep-libs png_static sksl svgdom etc1 SkKTX giflib webp_dec webp_dsp webp_enc webp_demux 
	webp_dsp_enc webp_utils raw_codec dng_sdk piex zlib)
foreach(lib ${dep-libs})
	find_library(Skia_LIBRARY_${lib}
		${lib}
		PATH ${Skia_BUILD_DIR}/out/Debug
		PATH ${Skia_BUILD_DIR}/out/Debug/obj/gyp)
	list(APPEND Skia_LIBRARIES ${Skia_LIBRARY_${lib}})
	list(APPEND skia-lib-check Skia_LIBRARY_${lib})
	mark_as_advanced(Skia_LIBRARY_${lib})
endforeach()

list(APPEND Skia_LIBRARIES ${Skia_LIBRARY_core})

set(skia-include core effects gpu images pathops private svg utils views xml ports codec config)
foreach(inc ${skia-include})
	set(Skia_INCLUDE_DIR_${inc} "${Skia_INCLUDE_DIR}/${inc}")
	list(APPEND Skia_INCLUDE_DIRS ${Skia_INCLUDE_DIR_${inc}})
	list(APPEND skia-inc-check Skia_INCLUDE_DIR_${inc})
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Skia DEFAULT_MSG ${skia-inc-check} ${skia-lib-check})
