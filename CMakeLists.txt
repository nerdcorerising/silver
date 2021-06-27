cmake_minimum_required(VERSION 3.1)

project(silver)

set (CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE $ENV{__CMakeBuildType})
set(CMAKE_INSTALL_PREFIX $ENV{__CMakeBinDir})

if (APPLE)
    # It would be great if this wasn't hardcoded...
    set(LLVM_DIR /usr/local/Cellar/llvm/12.0.0_1/)
    
    include_directories(${LLVM_DIR}/include/)
    link_directories(${LLVM_DIR}/lib/)
    set(ADDITIONAL_DEPS z curses m ffi)
endif (APPLE)

if (WIN32)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    include_directories("C:/personal/llvm_bin/LLVM_Debug_x64/include")
    link_directories("C:/personal/llvm_bin/LLVM_Debug_x64/lib")
else (CMAKE_BUILD_TYPE STREQUAL "Debug")
    include_directories("C:/personal/llvm_bin/LLVM_Release_x64/include")
    link_directories("C:/personal/llvm_bin/LLVM_Release_x64/lib")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")
endif (WIN32)

set(DEPENDENCIES LLVM-12 ${ADDITIONAL_DEPS})

add_subdirectory("src")

