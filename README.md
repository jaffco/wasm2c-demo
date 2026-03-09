# wasm2c Demo for Daisy Seed

This project demonstrates embedding WebAssembly on a Daisy Seed using [wasm2c](https://github.com/WebAssembly/wabt/blob/main/wasm2c/README.md) — a tool that converts a WASM binary into equivalent C source code, which is then compiled natively for the ARM Cortex-M7.

## Quick Start

1. First, [install the Daisy Toolchain](https://daisy.audio/tutorials/cpp-dev-env/#1-install-the-toolchain) && [Emscripten](https://emscripten.org) && [WABT](https://github.com/WebAssembly/wabt/blob/main/README.md#installing-prebuilt-binaries)
2. Once installed, use `./init.sh` to configure your local copy of this repository.
3. This repository is configured for building SRAM apps. Connect your Daisy Seed via USB and [install a bootloader](https://flash.daisy.audio/) before proceeding.
4. With your device in program mode, use `./run.sh` (or `SHIFT+CMD+B` in VSCode) to build, flash, and run the program. Output will be logged to `log.txt`.

## Project Structure

```
wasm2c-demo/
├── src/
│   ├── main.cpp              # Main application with wasm2c integration
│   └── SDRAM.hpp             # Custom SDRAM allocator for wasm memory
├── wasm-module/
│   ├── build/
│   │   ├── module.wasm       # Compiled WASM bytecode
│   │   ├── wasm-module_0.c   # Translated C source (split across 8 files)
│   │   ├── ...
│   │   ├── wasm-module_7.c
│   │   ├── wasm-module.h     # Generated module header
│   │   └── wasm-module-impl.h
│   ├── module.cpp            # Module source code
│   └── build-wasm.sh         # Module build script
├── wasm2c-runtime/           # wasm2c runtime submodule
├── libDaisy/                 # libDaisy submodule
├── common.mk                 # Shared build configuration
└── Makefile                  # Main build system
```

## Expected Output

Connect via USB serial to see:
- Runtime initialization
- Module instantiation
- Function execution tests
- Benchmark results with timing metrics
- Real-time performance analysis

## wasm2c Configuration

This build uses:
- **wasm2c** to translate WASM bytecode to portable C source (split into 8 output files)
- **wasm2c-runtime** for the sandboxed execution environment
- **SDRAM allocator** to handle wasm linear memory beyond SRAM limits
- **Cortex-M7 optimization** with FPU support

## Modifying the Module

Edit `wasm-module/module.cpp` and rebuild:

```bash
cd wasm-module
./build-wasm.sh
cd ..
make clean && make
```

## Troubleshooting

**"emcc not found"**
- Install Emscripten: `brew install emscripten`

**"wasm2c not found"**
- Install WABT: `brew install wabt`

**Build errors**
- Ensure submodules are initialized: `./init.sh`
- Check ARM toolchain is installed: `arm-none-eabi-gcc --version`

## Benchmarking

Checkout `FAUST`, `GEN`, `KLON`, and `NAM` branches to compare performance across different WASM module code.