# Simple Rust Compiler

A simple Rust compiler implemented in C++20, generating LLVM IR.

## Features

- **Lexer**: Tokenizes Rust source code
- **Parser**: Builds Abstract Syntax Tree (AST)
- **Semantic Analysis**: Name resolution, type checking, and type inference
- **IR Generation**: Generates LLVM IR text format

### Supported Rust Features

- **Types**: `i32`, `u32`, `isize`, `usize`, `bool`, arrays `[T; N]`, structs
- **Variables**: `let`, `let mut`, variable shadowing
- **Control Flow**: `if`/`else`, `while`, `loop`, `break`, `continue`
- **Functions**: Function definitions, parameters, return values, nested functions
- **Structs**: Definition, initialization, field access, methods (`impl` blocks)
- **Operators**: Arithmetic, comparison, logical (`&&`, `||` with short-circuit), bitwise
- **References**: `&T`, `&mut T`

## Project Structure

```
src/
├── main.cpp              # Entry point
├── lexer/                # Lexical analysis
├── parser/               # Syntax analysis (AST construction)
├── ast/                  # AST node definitions
├── semantic/             # Semantic analysis
│   ├── name_resolution   # Symbol table & scope management
│   ├── type_check        # Type checking
│   └── type_resolve      # Type inference
├── ir/                   # LLVM IR generation
│   ├── ir_generator_*    # IR generation (visitor pattern)
│   ├── ir_emitter        # IR text emission
│   ├── type_mapper       # Rust type → LLVM type mapping
│   └── value_manager     # Variable & scope management
├── pre_processor/        # Source preprocessing
├── error/                # Error handling
└── tool/                 # Utility functions
```

## Build

### Requirements

- CMake ≥ 3.22.1
- C++20 compatible compiler (GCC 11+ recommended)

### Compile

```bash
mkdir build && cd build
cmake ..
make -j4
```

### Run

```bash
./code <source_file.rs
```

