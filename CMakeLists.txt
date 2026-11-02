cmake_minimum_required(VERSION 3.12)
project(pioneer_trail C)

if (NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "GCC is required to build this project. Current compiler: ${CMAKE_C_COMPILER_ID}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Curses REQUIRED)

file(GLOB SOURCES "src/*.c")

add_executable(pioneer-trail ${SOURCES})

target_include_directories(pioneer-trail PRIVATE ${CURSES_INCLUDE_DIRS} src)
target_link_libraries(pioneer-trail PRIVATE ${CURSES_LIBRARIES} m)

target_compile_options(pioneer-trail PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
)

install(TARGETS pioneer-trail DESTINATION bin)