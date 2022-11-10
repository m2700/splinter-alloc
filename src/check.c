#include <splinter-alloc.h>

#include "check.h"
#include "debug.h"
#include "iter_free_blocks.h"

#if SPLA_TESTING

void spla_check(splinter_alloc *spla_alloc) {
    void *last_limit = NULL;
    FOREACH_FREE_BLOCK(free_blocks, spla_alloc, min_fl_idx) {
        if (last_limit > (void *)free_blocks[min_fl_idx]) {
            DBG_ERR2(-1, overlapping_free_blocks,
                     ("0x%012lx..", (size_t)last_limit),
                     ("0x%012lx..", (size_t)free_blocks[min_fl_idx]));
        }
        last_limit =
            (void *)free_blocks[min_fl_idx] + FL_IDX_TO_BLCK_SIZE(min_fl_idx);
    }
}

#endif // SPLA_TESTING