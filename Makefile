# Project Name
TARGET = main

# Sources
CPP_SOURCES = src/main.cpp
CPP_SOURCES += wasm-module/Phhhsrrr/gen_dsp/genlib.cpp

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
C_INCLUDES += -Iwasm-module/Phhhsrrr/gen_dsp
C_INCLUDES += -Iwasm-module/Phhhsrrr

# Defines
C_DEFS += -DGENLIB_NO_JSON
C_DEFS += -DGENLIB_USE_FLOAT32
C_DEFS += -DGENLIB_NO_DENORM_TEST
C_DEFS += -DWIN32

# Library Locations
include common.mk

# Override compilation of genlib.cpp to force-include headers in dependency order:
# genlib.h sets up macros (pow→fasterpow), genlib_exportfunctions.h declares systime_ticks
# etc., then genlib_ops.h can finally define fasterpow with all symbols available.
build/genlib.o: wasm-module/Phhhsrrr/gen_dsp/genlib.cpp Makefile | build
	$(CXX) -c $(CPPFLAGS) $(CPP_STANDARD) \
		-include genlib.h \
		-include genlib_exportfunctions.h \
		-include genlib_ops.h \
		-Wa,-a,-ad,-alms=build/genlib.lst $< -o $@