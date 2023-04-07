#pragma once

#include "alloc.h"
#include "config.h"

#if SPLA_AVL_FREE_LISTS
#include "avl.h"

typedef spla_avl_node iter_block;
#else  // SPLA_AVL_FREE_LISTS
typedef spla_block iter_block;
#endif // SPLA_AVL_FREE_LISTS
static inline size_t __first_block_idx(iter_block *blocks[SPLA_NUM_BLOCK_SIZES]) {
    size_t min_fl_idx = 0;
    for (size_t fl_i = 1; fl_i < SPLA_NUM_BLOCK_SIZES; fl_i++) {
        if (blocks[fl_i] != NULL
            && (blocks[fl_i] < blocks[min_fl_idx] || blocks[min_fl_idx] == NULL))
        {
            min_fl_idx = fl_i;
        }
    }
    return min_fl_idx;
}
#undef iter_block

#if SPLA_AVL_FREE_LISTS
#define FOREACH_FREE_BLOCK(free_blocks, spla_alloc, min_fl_idx)                                    \
    spla_avl_node *free_blocks[SPLA_NUM_BLOCK_SIZES];                                              \
    for (size_t fl_i = 0; fl_i < SPLA_NUM_BLOCK_SIZES; fl_i++) {                                   \
        free_blocks[fl_i] = spla_alloc->free_blocks[fl_i]->first;                                   \
    }                                                                                              \
    for (size_t min_fl_idx = __first_block_idx(free_blocks); free_blocks[min_fl_idx] != NULL; ({   \
             free_blocks[min_fl_idx] = free_blocks[min_fl_idx]->next;                              \
             min_fl_idx = __first_block_idx(free_blocks);                                          \
         }))
#else // SPLA_AVL_FREE_LISTS
#define FOREACH_FREE_BLOCK(free_blocks, spla_alloc, min_fl_idx)                                    \
    spla_block *free_blocks[SPLA_NUM_BLOCK_SIZES];                                                 \
    for (size_t fl_i = 0; fl_i < SPLA_NUM_BLOCK_SIZES; fl_i++) {                                   \
        free_blocks[fl_i] = spla_alloc->free_blocks[fl_i];                                         \
    }                                                                                              \
    for (size_t min_fl_idx = __first_block_idx(free_blocks); free_blocks[min_fl_idx] != NULL; ({   \
             free_blocks[min_fl_idx] = free_blocks[min_fl_idx]->next;                              \
             min_fl_idx = __first_block_idx(free_blocks);                                          \
         }))
#endif // SPLA_AVL_FREE_LISTS