cmake_minimum_required(VERSION 3.1)

project(silver)

include_directories(".")

set(SOURCES main.cpp ast/ast.cpp codegen/codegen.cpp parser/parser.cpp parser/tokenizer.cpp parser/tokenmanager.cpp passes/optimizationpass.cpp passes/hoistdeclarationpass.cpp)

add_executable(silver ${SOURCES})

target_link_libraries(silver ${DEPENDENCIES})

install(TARGETS silver DESTINATION ${CMAKE_INSTALL_PREFIX})
