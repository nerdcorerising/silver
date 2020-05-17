cmake_minimum_required(VERSION 3.1)

include_directories(".")

set(PARSER_SOURCES parser/parser.cpp parser/tokenizer.cpp parser/tokenmanager.cpp)
set(AST_SOURCES ast/ast.cpp)
set(ANALYSIS_SOURCES passes/analysispass.cpp passes/hoistdeclarationpass.cpp passes/typeinferencepass.cpp passes/referencecounting.cpp)
set(CODEGEN_SOURCES codegen/codegen.cpp)


set(SOURCES main.cpp ${PARSER_SOURCES} ${AST_SOURCES} ${ANALYSIS_SOURCES} ${CODEGEN_SOURCES})

add_executable(silver ${SOURCES})

target_link_libraries(silver ${DEPENDENCIES})

install(TARGETS silver DESTINATION ${CMAKE_INSTALL_PREFIX})
