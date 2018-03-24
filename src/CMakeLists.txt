cmake_minimum_required(VERSION 3.1)

project(silver)

set(SOURCES main.cpp ast.cpp codegen.cpp parser.cpp tokenizer.cpp)

add_executable(silver ${SOURCES})

target_link_libraries(silver ${DEPENDENCIES})

install(TARGETS silver DESTINATION ${CMAKE_INSTALL_PREFIX})
