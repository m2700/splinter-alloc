#include <assert.h>
#include <splinter-alloc.h>
#include <sys/param.h>

#include "align.h"
#include "alloc.h"
#include "check.h"
#include "config.h"
#include "debug.h"
#include "free.h"

static void spla_insert_free_block(splinter_alloc *spla_alloc, void *ptr, unsigned blck_align) {
    if (blck_align >= SPLA_PAGE_SHIFT) {
        size_t page_num = (size_t)1 << (blck_align - SPLA_PAGE_SHIFT);
        DBG_FN2(return_pages, ("0x%012lx", (size_t)ptr), ("%lu", page_num));
        spla_alloc->pfree_f(spla_alloc->pallocator, ptr, page_num);
        return;
    }

    unsigned fl_idx = BLCK_ALIGN_TO_FL_IDX(blck_align);
    size_t blck_size = (size_t)1 << blck_align;
    DBG_FN2(insert_free_block, ("0x%012lx", (size_t)ptr), ("0x%lx", blck_size));

    char compact_first = MAX_ALIGN_OF(ptr) > blck_align;

#if SPLA_SORT_COMPACT_TRIES != -1
    size_t sort_compact_tries_left = SPLA_SORT_COMPACT_TRIES;
#endif

    spla_block **block;
    for (block = &spla_alloc->free_blocks[fl_idx]; *block != NULL; block = &(*block)->next) {
#if SPLA_SORT_COMPACT_TRIES != -1
        if (sort_compact_tries_left-- == 0) {
            break;
        }
#endif

        if (!compact_first && (char *)*block + blck_size == ptr) {
            DBG_FN2(block_compaction, ("0x%012lx..", (size_t)*block), ("0x%012lx..", (size_t)ptr));

            SPLA_LOCK_ATOMIC;
            ptr = *block;
            *block = (*block)->next;
            spla_check(spla_alloc);
            SPLA_UNLOCK_ATOMIC;

            return spla_insert_free_block(spla_alloc, ptr, blck_align + 1);
        } else if (compact_first && ptr + blck_size == *block) {
            DBG_FN2(block_compaction, ("0x%012lx..", (size_t)ptr), ("0x%012lx..", (size_t)*block));

            SPLA_LOCK_ATOMIC;
            *block = (*block)->next;
            spla_check(spla_alloc);
            SPLA_UNLOCK_ATOMIC;

            return spla_insert_free_block(spla_alloc, ptr, blck_align + 1);
        }
#if SPLA_TESTING
        else if (((char *)*block < ptr && (char *)*block + blck_size > ptr)
                 || (ptr < (char *)*block && ptr + blck_size > (void *)*block))
        {
            DBG_ERR2(-1, overlapping_free_blocks, ("0x%012lx..", (size_t)ptr),
                     ("0x%012lx..", (size_t)*block));
        }
#endif
        else if ((void *)*block > ptr)
        {
            break;
        }
    }

    SPLA_LOCK_ATOMIC;
    spla_block *next_block = *block;
    *block = ptr;
    (*block)->next = next_block;
    spla_check(spla_alloc);
    SPLA_UNLOCK_ATOMIC;
}

void spla_free_area(splinter_alloc *spla_alloc, void *ptr, void *limit) {
    DBG_FN2(free_area, ("0x%012lx", (size_t)ptr), ("0x%012lx", (size_t)limit));
    assert(limit - ptr >= SPLA_MIN_BLOCK_SIZE);
    assert(MAX_ALIGN_OF(ptr) >= SPLA_MIN_ALIGNMENT_SHIFT);
    assert(MAX_ALIGN_OF(limit) >= SPLA_MIN_ALIGNMENT_SHIFT);

    do {
        unsigned ptr_align = MAX_ALIGN_OF(ptr);
        unsigned max_align = LE_POW2_SHIFT(limit - ptr);
        unsigned blck_align = MIN(ptr_align, max_align);
        spla_insert_free_block(spla_alloc, ptr, blck_align);
        size_t blck_size = (size_t)1 << blck_align;
        ptr += blck_size;
    } while (ptr < limit);
}

void spla_free(splinter_alloc *spla_alloc, void *ptr) {
    if (ptr == NULL) {
        return;
    }
    ptr -= sizeof(size_t);
    size_t alloc_size = *(size_t *)ptr + sizeof(size_t);
    spla_free_area(spla_alloc, ptr, ptr + alloc_size);
}