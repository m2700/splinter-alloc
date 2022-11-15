#pragma once

#include <splinter-alloc.h>
#include <stddef.h>
#include <strings.h>

#include "bitidx.h"
#include "config.h"

typedef struct spla_block spla_block;
struct spla_block {
    spla_block *next;
};

#define SPLA_NUM_BLOCK_SIZES                                                   \
    (SPLA_MAX_ALIGNMENT_SHIFT - SPLA_MIN_ALIGNMENT_SHIFT + 1)

typedef struct splinter_alloc {
    void *pallocator;
    spla_palloc_func_t palloc_f;
    spla_pfree_func_t pfree_f;
    spla_block *free_blocks[SPLA_NUM_BLOCK_SIZES];
} splinter_alloc;

#define SHIFT_TO_REST_MASK(shift) (((size_t)1 << (shift)) - 1)
#define SHIFT_TO_MASK(shift) ~SHIFT_TO_REST_MASK(shift)
#define ALIGN_UP(x, shift)                                                     \
    (typeof(x))(((size_t)(x) + SHIFT_TO_REST_MASK(shift)) &                    \
                SHIFT_TO_MASK(shift))
#define ALIGN_UP_SIZE(x, align_size)                                           \
    (typeof(x))(((size_t)(x) + ((align_size)-1)) & ~((align_size)-1))
#define DIV_SHIFT_CEIL(x, shift)                                               \
    ((((x) + SHIFT_TO_REST_MASK(shift)) >> (shift)))

#define FL_IDX_TO_BLCK_ALIGN(fl_i) (SPLA_MIN_ALIGNMENT_SHIFT + (fl_i))
#define FL_IDX_TO_BLCK_SIZE(fl_i) ((size_t)1 << FL_IDX_TO_BLCK_ALIGN(fl_i))
#define BLCK_ALIGN_TO_FL_IDX(blck_align) ((blck_align)-SPLA_MIN_ALIGNMENT_SHIFT)

/// x must not be 0
#define GE_POW2_SHIFT(x) ((x) == 1 ? 0 : sizeof(long) * 8 - clz((long)(x) - 1))
/// returns 0 for x = 0
#define GE_POW2(x) ((x) == 0 ? 0 : (size_t)1 << GE_POW2_SHIFT(x))

/// x must not be 0
#define LE_POW2_SHIFT(x) (sizeof(long) * 8 - clz((long)(x)) - 1)
/// returns 0 for x = 0
#define LE_POW2(x) ((x) == 0 ? 0 : (size_t)1 << LE_POW2_SHIFT(x))

#define MAX_ALIGN_OF(x) ((x) == 0 ? sizeof(size_t) * 8 - 1 : ctz((long)(x)))