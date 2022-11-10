#include <splinter-alloc.h>
#include <stdio.h>

#include "alloc.h"
#include "iter_free_blocks.h"

void spla_dump(splinter_alloc *spla_alloc) {
    printf("free ranges:\n");

    void *last_limit = NULL;
    FOREACH_FREE_BLOCK(free_blocks, spla_alloc, min_fl_idx) {
        void *limit =
            (void *)free_blocks[min_fl_idx] + FL_IDX_TO_BLCK_SIZE(min_fl_idx);

        if (last_limit != NULL &&
            last_limit == (void *)free_blocks[min_fl_idx]) {
            printf("                ");
        } else {
            if (last_limit != NULL) {
                printf("\n");
            }
            printf("  0x%012lx", (size_t)free_blocks[min_fl_idx]);
        }
        printf(" ..= 0x%012lx\n", (size_t)limit);

        last_limit = limit;
    }
}