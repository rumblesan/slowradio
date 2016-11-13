
include_directories(include)

set(SOURCES "src/main.c")

add_executable(slowradio ${SOURCES})

target_link_libraries(slowradio slow)

install(TARGETS slowradio EXPORT SlowRadioTargets
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
