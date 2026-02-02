# wasm2c-demo

This project demonstrates a workflow for getting [WebAssembly](https://en.wikipedia.org/wiki/WebAssembly) applications running as native machine code on the Daisy Seed.

It is based on [embedded-wasm-apps](https://github.com/wasm3/embedded-wasm-apps), a project from [Wasm3 Labs](https://github.com/wasm3).

## Quick Start

1. First, [install the Daisy Toolchain](https://daisy.audio/tutorials/cpp-dev-env/#1-install-the-toolchain) && [Emscripten](https://emscripten.org) && [WABT](https://github.com/WebAssembly/wabt/blob/main/README.md#installing-prebuilt-binaries)
2. Once installed, use `./init.sh` to configure your local copy of this repository. 
3. This repository is configured for building SRAM apps. Connect your Daisy Seed via USB and [install a bootloader](https://flash.daisy.audio/) before proceeding.
4. With your device in program mode, use `./run.sh` (or `SHIFT+CMD+B` in VSCode) to build, flash, and run the program. Output will be logged to `log.txt`.