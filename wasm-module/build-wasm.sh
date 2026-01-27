#!/bin/bash
set -e

WAMR_ROOT=../wasm-micro-runtime

echo "Building WASM module..."

# Clean old build artifacts
rm -f module.wasm module.aot module_aot.h

# Create build directory
mkdir -p build

# Check for emcc
if ! command -v emcc &> /dev/null; then
    echo "ERROR: emscripten not found!"
    echo "Please install with: brew install emscripten"
    exit 1
fi

echo "Using emscripten: $(which emcc)"

# Compile C++ to WASM using emscripten
echo "Step 1: Compiling C++ to WASM..."
emcc \
    -O2 \
    -sSTANDALONE_WASM \
    -sEXPORTED_RUNTIME_METHODS=[] \
    -sEXPORTED_FUNCTIONS=_process \
    -sERROR_ON_UNDEFINED_SYMBOLS=0 \
    --no-entry \
    -o build/module.wasm \
    module.cpp

echo "WASM module size: $(wc -c < build/module.wasm) bytes"

# Convert wasm 2 C source
echo "Step 2: Converting WASM to C source..."
wasm2c --no-debug-names --module-name="module" --num-outputs=8 build/module.wasm -o build/wasm-module.c
echo "WASM app compiled!"

echo ""
echo "================================"
echo "Module build complete!"
echo "================================"
echo "Generated files:"
echo "  - build/module.wasm ($(wc -c < build/module.wasm) bytes)"
echo "  - build/wasm-module_0.c ($(wc -c < build/wasm-module_0.c) bytes)"
echo "  - build/wasm-module_1.c ($(wc -c < build/wasm-module_1.c) bytes)"
echo "  - build/wasm-module_2.c ($(wc -c < build/wasm-module_2.c) bytes)"
echo "  - build/wasm-module_3.c ($(wc -c < build/wasm-module_3.c) bytes)"
echo "  - build/wasm-module_4.c ($(wc -c < build/wasm-module_4.c) bytes)"
echo "  - build/wasm-module_5.c ($(wc -c < build/wasm-module_5.c) bytes)"
echo "  - build/wasm-module_6.c ($(wc -c < build/wasm-module_6.c) bytes)"
echo "  - build/wasm-module_7.c ($(wc -c < build/wasm-module_7.c) bytes)"
echo "  - build/wasm-module.h ($(wc -c < build/wasm-module.h) bytes)"
echo "  - build/wasm-module-impl.h ($(wc -c < build/wasm-module-impl.h) bytes)"
echo ""
