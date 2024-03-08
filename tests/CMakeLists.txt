set(SRC
  ofono.c
  tst_blocker.c)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_RPATH "${PROJECT_BINARY_DIR}/src")

add_executable(tst_blocker ${SRC})

target_include_directories(tst_blocker PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(tst_blocker
  PkgConfig::GOBJECT
  PkgConfig::GIO
  voicecallfilter)

add_test(NAME tst_blocker COMMAND tst_blocker)
