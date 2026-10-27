# patch.Init() firmware. Requires the ARM toolchain (arm-none-eabi-gcc) and
# libDaisy checked out at ../lib/libDaisy (git submodule, see README).
#
#   make -C lib/libDaisy        # once
#   make -C hw                  # build -> build/doombringer.bin
#   make -C hw program-dfu      # flash over USB (hold BOOT, tap RESET first)

TARGET        = doombringer
CPP_SOURCES   = main.cpp
CPP_STANDARD  = -std=gnu++17
OPT           = -O2

# make CPU_METER=1 prints DSP load over USB serial once a second.
CPU_METER    ?= 0
C_DEFS       += -DCPU_METER=$(CPU_METER)

LIBDAISY_DIR     = ../lib/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
