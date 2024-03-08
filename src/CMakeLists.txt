set(SRC
  plugin.c
  blocker.c blocker.h)

add_library(voicecallfilter SHARED ${SRC})

add_definitions(-DVERSION=\"${PROJECT_VERSION}\")

target_link_libraries(voicecallfilter
  PkgConfig::GOBJECT
  PkgConfig::GIO)

pkg_get_variable(OFONO_PLUGINDIR ofono plugindir)
install(TARGETS voicecallfilter
	DESTINATION ${OFONO_PLUGINDIR})

pkg_get_variable(GIO_SCHEMASDIR gio-2.0 schemasdir)
install(FILES sailfish.voicecall.filter.gschema.xml
	DESTINATION ${GIO_SCHEMASDIR})
