# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Silver is a toy compiled language using LLVM as its backend. It aims to provide cleaner syntax than C/C++ without garbage collection or runtime. Currently supports basic features: functions, integer/float math, type inference for numeric types, if statements, and while loops.

## Build Commands

```bash
# Install LLVM via vcpkg
vcpkg install llvm

# Configure (from repository root, using vcpkg toolchain)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build

# The executable is output to build/src/compiler/silver (or silver.exe on Windows)
```

### Prerequisites

- **vcpkg** with LLVM installed (`vcpkg install llvm`)

## Running Tests

```bash
cd src/test
python test.py
```

Tests run all `.sl` files in `src/test/programs/` and expect exit code 50.

## Running the Compiler

```bash
./silver <source_file.sl> [-jit] [-bytecode] [-debug|-release]
```

- Default mode runs with JIT
- `-bytecode` outputs to `sampleoutput.bc`

## Architecture

The compiler follows a traditional pipeline:

1. **Tokenizer** (`parser/tokenizer.cpp`) - Lexical analysis
2. **Parser** (`parser/parser.cpp`) - Recursive descent parser producing AST
3. **Analysis Passes** (`passes/`) - Multiple passes over the AST:
   - `hoistdeclarationpass` - Declaration hoisting
   - `typeinferencepass` - Type inference for ints/floats
   - `referencecounting` - ARC pass (placeholder)
   - `analysispass` - Pass manager coordinates all passes
4. **Code Generation** (`codegen/codegen.cpp`) - LLVM IR generation and JIT execution

### Key Types

- `ast::Assembly` - Root node containing all functions
- `ast::Function` - Function definition with block body
- `ast::Expression` - Base class for all expressions (literals, binary ops, if/while, etc.)
- `SymbolTable<K,V>` - Scoped symbol table template used throughout
- `AnalysisPassManager` - Runs all analysis passes in order
- `CodeGen` - Generates LLVM IR and can run via JIT

### Source File Layout

- `src/compiler/main.cpp` - Entry point, orchestrates parsing → analysis → codegen
- `src/compiler/ast/` - AST node definitions
- `src/compiler/parser/` - Tokenizer and recursive descent parser
- `src/compiler/passes/` - Analysis and transformation passes
- `src/compiler/codegen/` - LLVM code generation
