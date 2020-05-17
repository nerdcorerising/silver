cmake_minimum_required(VERSION 3.1)

include_directories(".")

set(SOURCES main.cpp)

add_executable(test ${SOURCES})

target_link_libraries(test ${DEPENDENCIES})
