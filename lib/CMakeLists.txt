
include_directories(include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -D_POSIX_C_SOURCE=199309L")

set(SOURCES
  "src/filereader_process.c"
  "src/encoder_process.c"
  "src/ogg_encoder.c"
  "src/messages.c"
  "src/filechunk.c"
  "src/stretcher_process.c"
  "src/broadcast_process.c"
  "src/util.c"
  )

add_library(slow ${SOURCES})

target_link_libraries(slow shout m)
target_link_libraries(slow bclib)
target_link_libraries(slow pstretch fftw3f)
target_link_libraries(slow vorbis vorbisfile ogg vorbisenc)
target_link_libraries(slow pthread)
