
include_directories(include)

set(SOURCES "src/config.c" "src/main.c")

add_executable(slowradio ${SOURCES})

target_link_libraries(slowradio slow)

target_link_libraries(slowradio config)
