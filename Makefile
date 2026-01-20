# Project Name
TARGET = main

# Sources
CPP_SOURCES = src/main.cpp

# WASM-generated C files (in current directory)
C_SOURCES = \
	build/wasm-app_0.c \
	build/wasm-app_1.c \
	build/wasm-app_2.c \
	build/wasm-app_3.c \
	build/wasm-app_4.c \
	build/wasm-app_5.c \
	build/wasm-app_6.c \
	build/wasm-app_7.c

# WASM runtime C sources (in parent src directory)
C_SOURCES += \
	wasm2c-runtime/src/wasm-rt-impl.c \
	wasm2c-runtime/src/wasm-rt-exceptions-impl.c \
	wasm2c-runtime/src/wasm-rt-mem-impl.c

# Include directories
C_INCLUDES += -Iwasm2c-runtime/include
C_INCLUDES += -Ibuild # generated files

# Library Locations
include common.mk

.PHONY: app

app:
	cd app && chmod +x build.sh && ./build.sh
	# Create build directory if it doesn't exist
	mkdir -p build
	wasm2c --no-debug-names --module-name="app" --num-outputs=8 app/build/app.wasm -o build/wasm-app.c
	@echo "WASM app compiled!"

# daisy:
# 	-@rm apps/daisy-audio/audio.wasm
# 	cd apps/daisy-audio && chmod +x build.sh && ./build.sh
# 	-@rm ./daisy/wasm-app*
# 	wasm2c --no-debug-names --module-name="app" --num-outputs=8 apps/daisy-audio/audio.wasm -o daisy/wasm-app.c
# 	@echo "WASM app compiled! Now run: ./run.sh"