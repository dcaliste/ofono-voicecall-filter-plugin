cmake_minimum_required(VERSION 3.6)

project(ofono-voicecall-filter-plugin
	VERSION 0.1
	DESCRIPTION "A voice call filter for Ofono")

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OFONO ofono IMPORTED_TARGET REQUIRED)

add_subdirectory(src)
