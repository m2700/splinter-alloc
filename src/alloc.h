#pragma once

#include <splinter-alloc.h>
#include <stddef.h>
#include <string.h>

#include "bitidx.h"
#include "config.h"

#if SPLA_AVL_FREE_LISTS
#include "avl.h"
#endif // SPLA_AVL_FREE_LISTS

#if SPLA_AVL_FREE_LISTS
typedef spla_avl_tree spla_block;
#else  // SPLA_AVL_FREE_LISTS
typedef struct spla_block spla_block;
struct spla_block {
    spla_block *next;
};
#endif // SPLA_AVL_FREE_LISTS

#define SPLA_NUM_BLOCK_SIZES (SPLA_MAX_ALIGNMENT_SHIFT - SPLA_MIN_ALIGNMENT_SHIFT + 1)

typedef struct splinter_alloc {
    void *pallocator;
    spla_palloc_func_t palloc_f;
    spla_pfree_func_t pfree_f;
    spla_block *free_blocks[SPLA_NUM_BLOCK_SIZES];
} splinter_alloc;

#define FL_IDX_TO_BLCK_ALIGN(fl_i) (SPLA_MIN_ALIGNMENT_SHIFT + (fl_i))
#define FL_IDX_TO_BLCK_SIZE(fl_i) ((size_t)1 << FL_IDX_TO_BLCK_ALIGN(fl_i))
#define BLCK_ALIGN_TO_FL_IDX(blck_align) ((blck_align)-SPLA_MIN_ALIGNMENT_SHIFT)