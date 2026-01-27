# Silver

Silver is a toy compiled language using LLVM as its backend. The goal is to provide cleaner syntax than C/C++ without garbage collection or runtime overhead.

## Current Status

Silver is a working language with a reasonable feature set for experimentation and learning.

### What's Working

**Types & Variables**
- Primitive types: `int`, `float`, `string`, `void`
- Variable declarations with `let` and type inference for numeric literals
- Explicit type casting between int and float

**Functions**
- Function definitions with `fn` keyword
- Parameters with explicit types and return types
- Visibility modifiers (`public`, `private`)
- Namespace-qualified calls (e.g., `Math.add(1, 2)`)

**Classes**
- Class definitions with fields and methods
- Visibility modifiers for fields and methods
- Instance creation with `alloc ClassName(args...)`
- Field and method access via dot notation
- `this` reference in instance methods

**Control Flow**
- `if`/`elif`/`else` statements
- `while` loops
- Nested control structures

**Operators**
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `<`, `>`, `<=`, `>=`, `==`, `!=`
- Logical: `&&`, `||`

**Strings**
- UTF-8 encoded strings
- Escape sequences: `\n`, `\t`, `\r`, `\\`, `\"`, `\uXXXX`, `\U00XXXXXX`
- String functions: `strlen_utf8()`, `string_bytes()`, `strcmp()`

**Memory Management**
- Automatic Reference Counting (ARC)
- Objects cleaned up when reference count reaches 0

**Other**
- Namespaces (including nested)
- Import system with framework modules (`math`, `io`)
- Single-line comments with `#`
- JIT compilation (default) or bytecode output

### What's Not Implemented

- Arrays/collections
- Generics
- Inheritance
- Pattern matching
- For loops
- Error handling/exceptions

## Prerequisites

### Windows

- **LLVM** - Download pre-built binaries from [LLVM Releases](https://github.com/llvm/llvm-project/releases)
- **Visual Studio 2022** with C++ tools
- **CMake** and **Ninja**

## Building

### Windows

```cmd
# Set LLVM_DIR to point to the cmake config directory
set LLVM_DIR=C:\Program Files\LLVM\lib\cmake\llvm

# Run the build script
build.cmd
```

## Usage

```bash
./silver <source_file.sl> [-jit] [-bytecode] [-debug|-release]
```

- Default mode runs with JIT
- `-bytecode` outputs to `sampleoutput.bc`

## Example

```
fn main() -> int {
    let x = 10
    let y = 20

    if x < y {
        print_int(x + y)
    }

    return 0
}
```

## Running Tests

```bash
cd src/test
python test.py
```
