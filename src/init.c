#include <assert.h>
#include <splinter-alloc.h>
#include <string.h>

#include "alloc.h"
#include "config.h"
#include "debug.h"
#include "free.h"

splinter_alloc *spla_init_alloc(void *pallocator, size_t pallocator_size,
                                spla_palloc_func_t palloc_f, spla_pfree_func_t pfree_f) {
    size_t num_needed_pages = DIV_SHIFT_CEIL(sizeof(splinter_alloc), SPLA_PAGE_SHIFT);
    DBG_FN1(request_init_pages, ("%lu", num_needed_pages));
    void *free_mem_ptr = palloc_f(pallocator, num_needed_pages);
    assert(free_mem_ptr != NULL);
    void *free_limit = free_mem_ptr + (num_needed_pages << SPLA_PAGE_SHIFT);

    splinter_alloc *spla_alloc = free_mem_ptr;
    free_mem_ptr = spla_alloc + 1;
    free_mem_ptr = ALIGN_UP(free_mem_ptr, SPLA_MIN_ALIGNMENT_SHIFT);
    assert(free_mem_ptr <= free_limit);

    spla_alloc->palloc_f = palloc_f;
    spla_alloc->pfree_f = pfree_f;
    for (unsigned i = 0; i < SPLA_NUM_BLOCK_SIZES; i++) {
        spla_alloc->free_blocks[i] = NULL;
    }

    if (free_limit - free_mem_ptr >= SPLA_MIN_BLOCK_SIZE) {
        spla_free_area(spla_alloc, free_mem_ptr, free_limit);
    }

    if (pallocator != NULL && pallocator_size != 0) {
        spla_alloc->pallocator = spla_malloc(spla_alloc, pallocator_size);
        assert(spla_alloc->pallocator != NULL);
        memcpy(spla_alloc->pallocator, pallocator, pallocator_size);
    } else {
        spla_alloc->pallocator = pallocator;
    }

    return spla_alloc;
}