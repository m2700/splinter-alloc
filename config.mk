error_flags = -Wall -Werror
defs = -DSPLA_DEBUG=0 -DSPLA_TESTING=1 -DSPLA_ALLOCATE_EXACT=1
incls = -I$(SRC_DIR)/include

BUILD_DIR = build
SRC_DIR = src
SUB_SRC_DIRS =
C_FLAGS = -g3 -O0 $(incls) $(defs) $(error_flags)
LD_FLAGS = 
LANG := C
LD = $(CC)