MPY_DIR = libs/micropython
MOD = ngu

SRC = modngu.c

# Architecture to build for (x86, x64, armv7m, xtensa, xtensawin)
#ARCH ?= xtensawin
ARCH ?= x64

CFLAGS += -lgcc

CLEAN_EXTRA = $(MOD).mpy

# Include to get the rules for compiling and linking the module
include $(MPY_DIR)/py/dynruntime.mk

tags:
	ctags -f .tags *.[ch]
