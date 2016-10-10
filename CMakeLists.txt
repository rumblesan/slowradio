cmake_minimum_required(VERSION 3.0)
project(slowradio)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories(include)

set(SOURCES
  "src/filereader.c"
  "src/ogg_encoder.c"
  "src/filechunk.c"
  "src/floatchunk.c"
  "src/shoutcast.c"
  "src/virtual_ogg.c"
  "src/main.c"
  )

add_executable(slow ${SOURCES})
set_property(TARGET slow PROPERTY C_STANDARD 99)
target_link_libraries(slow shout)

find_package(BCLib REQUIRED)
include_directories("${BCLIB_INCLUDE_DIRS}")
target_link_libraries(slow bclib)

target_link_libraries(slow sndfile)

#option (WITH_TESTS "build tests" ON)

#if (WITH_TESTS)
#    enable_testing()
#    add_subdirectory(tests)
#endif()
