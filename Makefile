# Get the directory where this Makefile is located
CONFIG_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Library Locations (allow environment override)
RTNEURAL_DIR ?= $(CONFIG_DIR)wasm-module/RTNeural

# Normalize paths for cross-platform compatibility
normalize_path = $(subst \,/,$(1))
RTNEURAL_DIR := $(call normalize_path,$(RTNEURAL_DIR))

# Project Name
TARGET = main

# Sources
CPP_SOURCES = src/main.cpp
CPP_SOURCES += $(RTNEURAL_DIR)/RTNeural/RTNeural.cpp

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

# WASM runtime C++ sources (for WASI stubs)
CPP_SOURCES += \
	wasm2c-runtime/src/wasi-stubs.cpp

# Include directories
C_INCLUDES += -Iwasm2c-runtime/include
C_INCLUDES += -Iwasm-module/build # generated files

# Includes and flags for RTNeural
C_INCLUDES += -I$(RTNEURAL_DIR)
C_INCLUDES += -I$(RTNEURAL_DIR)/modules

# RTNeural compiler flags
CPPFLAGS += -DRTNEURAL_DEFAULT_ALIGNMENT=8 -DRTNEURAL_NO_DEBUG=1 -DRTNEURAL_USE_EIGEN=1

# Library Locations
include common.mk