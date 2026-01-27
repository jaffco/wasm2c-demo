# Project Name
TARGET = main

# Sources
CPP_SOURCES = src/main.cpp

# WASM-generated C files (in current directory)
C_SOURCES = \
	wasm-module/build/wasm-module_0.c \
	wasm-module/build/wasm-module_1.c \
	wasm-module/build/wasm-module_2.c \
	wasm-module/build/wasm-module_3.c \
	wasm-module/build/wasm-module_4.c \
	wasm-module/build/wasm-module_5.c \
	wasm-module/build/wasm-module_6.c \
	wasm-module/build/wasm-module_7.c

# WASM runtime C sources (in parent src directory)
C_SOURCES += \
	wasm2c-runtime/src/wasm-rt-impl.c \
	wasm2c-runtime/src/wasm-rt-exceptions-impl.c \
	wasm2c-runtime/src/wasm-rt-mem-impl.c

# Include directories
C_INCLUDES += -Iwasm2c-runtime/include
C_INCLUDES += -Iwasm-module/build # generated files

# Library Locations
include common.mk