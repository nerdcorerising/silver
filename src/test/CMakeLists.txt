cmake_minimum_required(VERSION 3.16)

add_executable(test_runner test_runner.cpp)

# PDB verification tool - needs LLVM PDB libraries
llvm_map_components_to_libnames(PDB_LIBS
    debuginfopdb
    object
    support
)

add_executable(pdb_test pdb_test.cpp)
target_link_libraries(pdb_test ${PDB_LIBS})
target_compile_definitions(pdb_test PRIVATE ${LLVM_COMPILE_DEFINITIONS})
target_include_directories(pdb_test PRIVATE ${LLVM_INCLUDE_DIRS})
