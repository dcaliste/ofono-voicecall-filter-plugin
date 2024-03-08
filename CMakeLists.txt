cmake_minimum_required(VERSION 3.6)

project(ofono-voicecall-filter-plugin
	VERSION 0.1
	DESCRIPTION "A voice call filter for Ofono")

find_package(PkgConfig REQUIRED)
pkg_check_modules(OFONO ofono REQUIRED)
pkg_check_modules(GOBJECT gobject-2.0 IMPORTED_TARGET REQUIRED)
pkg_check_modules(GIO gio-2.0 IMPORTED_TARGET REQUIRED)

include(CTest)

add_subdirectory(src)
add_subdirectory(tests)
