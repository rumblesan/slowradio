
include_directories(include)

set(SOURCES
  "main.c"
  )

add_executable(slow ${SOURCES})
set_property(TARGET slow PROPERTY C_STANDARD 99)


