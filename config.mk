error_flags = -Wall -Werror
defs = -DSPLA_DEBUG=0 -DSPLA_TESTING=0 -DSPLA_ALLOCATE_EXACT=0 -DSPLA_SORT_COMPACT_TRIES=1 -DSPLA_AVL_FREE_LISTS=0
incls = -I$(SRC_DIR)/include

BUILD_DIR = build
SRC_DIR = src
SUB_SRC_DIRS =
C_FLAGS = -g3 -O0 $(incls) $(defs) $(error_flags)
LD_FLAGS = 
LANG := C
LD = $(CC)