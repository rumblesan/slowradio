cmake_minimum_required(VERSION 3.0)
project(slowradio)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories(include)

set(SOURCES
  "src/filereader.c"
  "src/encoder_process.c"
  "src/ogg_encoder.c"
  "src/messages.c"
  "src/filechunk.c"
  "src/stretcher.c"
  "src/shoutcast.c"
  "src/config.c"
  "src/main.c"
  "src/util.c"
  )

add_executable(slow ${SOURCES})
target_link_libraries(slow shout m)

target_link_libraries(slow bclib)
target_link_libraries(slow pstretch fftw3f)
target_link_libraries(slow config)
target_link_libraries(slow vorbis vorbisfile ogg vorbisenc)
target_link_libraries(slow pthread)

install(TARGETS slow
  DESTINATION bin)

#option (WITH_TESTS "build tests" ON)

#if (WITH_TESTS)
#    enable_testing()
#    add_subdirectory(tests)
#endif()
