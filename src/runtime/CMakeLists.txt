cmake_minimum_required(VERSION 3.16)

# Build static library for linking into executables
add_library(silver_runtime STATIC runtime.cpp)

# Install the library alongside the compiler
install(TARGETS silver_runtime
    ARCHIVE DESTINATION .
)
