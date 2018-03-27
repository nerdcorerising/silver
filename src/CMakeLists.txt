cmake_minimum_required(VERSION 3.1)

project(silver)

include_directories(".")

set(PARSER_SOURCES parser/parser.cpp parser/tokenizer.cpp parser/tokenmanager.cpp)
set(AST_SOURCES ast/ast.cpp)
set(OPT_SOURCES passes/optimizationpass.cpp passes/hoistdeclarationpass.cpp passes/typeinferencepass.cpp)
set(CODEGEN_SOURCES codegen/codegen.cpp)


set(SOURCES main.cpp ${PARSER_SOURCES} ${AST_SOURCES} ${OPT_SOURCES} ${CODEGEN_SOURCES})

add_executable(silver ${SOURCES})

target_link_libraries(silver ${DEPENDENCIES})

install(TARGETS silver DESTINATION ${CMAKE_INSTALL_PREFIX})
