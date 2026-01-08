cmake_minimum_required(VERSION 3.16)

project(silver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use dynamic CRT (/MD for Release, /MDd for Debug)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(CMAKE_BUILD_TYPE $ENV{__CMakeBuildType})
set(CMAKE_INSTALL_PREFIX $ENV{__CMakeBinDir})

# Find LLVM - set LLVM_DIR to your LLVM installation if not found automatically
find_package(LLVM CONFIG REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Find DIA SDK from current Visual Studio installation (needed by LLVM on Windows)
if(MSVC)
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH MSVC_BIN_DIR)
    # Navigate from VC/Tools/MSVC/<version>/bin/Hostx64/x64 up to VS install root
    cmake_path(GET MSVC_BIN_DIR PARENT_PATH MSVC_PARENT)
    cmake_path(GET MSVC_PARENT PARENT_PATH MSVC_PARENT)
    cmake_path(GET MSVC_PARENT PARENT_PATH MSVC_PARENT)
    cmake_path(GET MSVC_PARENT PARENT_PATH MSVC_PARENT)
    cmake_path(GET MSVC_PARENT PARENT_PATH MSVC_PARENT)
    cmake_path(GET MSVC_PARENT PARENT_PATH VS_INSTALL_DIR)
    set(DIA_SDK_LIB_DIR "${VS_INSTALL_DIR}/DIA SDK/lib/amd64")
    if(EXISTS "${DIA_SDK_LIB_DIR}/diaguids.lib")
        message(STATUS "Found DIA SDK at: ${DIA_SDK_LIB_DIR}")
        link_directories("${DIA_SDK_LIB_DIR}")
    endif()
endif()

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

