# WASM Module Build Instructions

This directory contains the C++ module that will be compiled to WebAssembly (WASM) and then converted to C code using wasm2c for execution on the Daisy Seed.

## Prerequisites

1. **Emscripten** - Required for compiling C++ to WASM
   - Install with: `brew install emscripten`
   - Or follow installation instructions at https://emscripten.org/docs/getting_started/downloads.html

2. **wasm2c** - Tool to convert WASM to C code
   - Built from wabt (WebAssembly Binary Toolkit)
   - Install with: `brew install wabt` (includes wasm2c)
   - Or build from source: https://github.com/WebAssembly/wabt

## Building

```bash
./build-wasm.sh
```

This will:
1. Compile `module.cpp` to `build/module.wasm` using Emscripten
2. Convert `build/module.wasm` to C source files using wasm2c
3. Generate multiple `.c` and `.h` files in `build/` for integration with the main project

## Manual Build Steps

If you need to build manually:

```bash
# Step 1: C++ to WASM using Emscripten
emcc \
    -O2 \
    -s STANDALONE_WASM \
    -s EXPORTED_RUNTIME_METHODS=[] \
    -s EXPORTED_FUNCTIONS=_process \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    --no-entry \
    -o build/module.wasm \
    module.cpp

# Step 2: WASM to C using wasm2c
wasm2c --no-debug-names --module-name="module" --num-outputs=8 build/module.wasm -o build/wasm-module.c
```

## Output Files

The build process generates:
- `build/module.wasm` - The WebAssembly binary
- `build/wasm-module_0.c` through `build/wasm-module_7.c` - Split C implementation files
- `build/wasm-module.h` and `build/wasm-module-impl.h` - Header files for integration

These C files are included in the main project's build system for cross-compilation to ARM Cortex-M7.
