#pragma once

#include "alloc.h"

static inline size_t
__first_block_idx(spla_block *blocks[SPLA_NUM_BLOCK_SIZES]) {
    size_t min_fl_idx = 0;
    for (size_t fl_i = 1; fl_i < SPLA_NUM_BLOCK_SIZES; fl_i++) {
        if (blocks[fl_i] != NULL &&
            (blocks[fl_i] < blocks[min_fl_idx] || blocks[min_fl_idx] == NULL)) {
            min_fl_idx = fl_i;
        }
    }
    return min_fl_idx;
}

#define FOREACH_FREE_BLOCK(free_blocks, spla_alloc, min_fl_idx)                            \
    spla_block *free_blocks[SPLA_NUM_BLOCK_SIZES];                             \
    for (size_t fl_i = 0; fl_i < SPLA_NUM_BLOCK_SIZES; fl_i++) {               \
        free_blocks[fl_i] = spla_alloc->free_blocks[fl_i];                     \
    }                                                                          \
    for (size_t min_fl_idx = __first_block_idx(free_blocks);                   \
         free_blocks[min_fl_idx] != NULL; ({                                   \
             free_blocks[min_fl_idx] = free_blocks[min_fl_idx]->next;          \
             min_fl_idx = __first_block_idx(free_blocks);                      \
         }))
