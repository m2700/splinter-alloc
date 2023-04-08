#include <splinter-alloc.h>

#include "check.h"
#include "config.h"
#include "debug.h"
#include "iter_free_blocks.h"

#if SPLA_TESTING

void spla_check(splinter_alloc *spla_alloc) {

    if (SPLA_SORT_COMPACT_TRIES != -1) {
        // FIXME: support partially sorted free lists
        return;
    }

    void *last_limit = NULL;
    FOREACH_FREE_BLOCK(free_blocks, spla_alloc, min_fl_idx) {
        if (last_limit > (void *)free_blocks[min_fl_idx]) {
            DBG_ERR2(-1, overlapping_free_blocks, ("0x%012lx..", (size_t)last_limit),
                     ("0x%012lx..", (size_t)free_blocks[min_fl_idx]));
        }
        last_limit = (void *)free_blocks[min_fl_idx] + FL_IDX_TO_BLCK_SIZE(min_fl_idx);
    }

#if SPLA_AVL_FREE_LISTS
    for (size_t fl_i = 0; fl_i < SPLA_NUM_BLOCK_SIZES; fl_i++) {
        spla_avl_tree_check(spla_alloc->free_blocks[fl_i]);
    }
#endif // SPLA_AVL_FREE_LISTS
}

#endif // SPLA_TESTING