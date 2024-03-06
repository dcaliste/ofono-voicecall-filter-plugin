set(SRC
	plugin.c)

add_library(voicecallfilter ${SRC})

add_definitions(-DVERSION=\"${PROJECT_VERSION}\")

target_link_libraries(voicecallfilter)

pkg_get_variable(OFONO_PLUGINDIR ofono plugindir)
install(TARGETS voicecallfilter
	DESTINATION ${OFONO_PLUGINDIR})
