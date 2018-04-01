# Intro
Silver is a toy language that is compiled with llvm. The end goal would be to have a compiled language with a cleaner syntax than C/C++, but no garbage collector or runtime.

# Current Status
It currently works in the sense that some programs can be run in it. It would not be a fun language to write anything in, however, because it lacks almost everything necessary to actually do anything useful.

What's working

    * Functions
    * Integer and floating point basic math (i.e. *, /, +, -, etc)
    * Type inference for ints and floats
    * If statements
    * While loops

What's not implemented

    * Error checking (makes it really difficult to write programs)
    * User defined types
    * Allocating heap memory
    * Any string operations besides just allocating a string
    * Type inference for anything besides ints or floats
    * Any sort of standard library

# Prerequisites
Silver requires llvm to run. This was developed on a mac, so using homebrew the only prereqs can be installed with `brew install llvm`
