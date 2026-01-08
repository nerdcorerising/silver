cmake_minimum_required(VERSION 3.16)

project(silver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE $ENV{__CMakeBuildType})
set(CMAKE_INSTALL_PREFIX $ENV{__CMakeBinDir})

find_package(LLVM CONFIG REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    executionengine
    interpreter
    support
    bitwriter
    passes
)

set(DEPENDENCIES ${LLVM_LIBS})

add_subdirectory("src")

