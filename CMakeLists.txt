cmake_minimum_required(VERSION 3.0)

project(slowradio)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -D_POSIX_C_SOURCE=199309L")

include_directories(lib/include)

add_subdirectory(lib)
add_subdirectory(main)

enable_testing()
add_subdirectory(tests)
