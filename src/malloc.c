#include <assert.h>
#include <splinter-alloc.h>

#include "alloc.h"
#include "check.h"
#include "debug.h"
#include "free.h"

static void *spla_pop_free_block(splinter_alloc *spla_alloc, unsigned *fl_idx) {
    if (*fl_idx >= SPLA_NUM_BLOCK_SIZES) {
        unsigned blck_size = FL_IDX_TO_BLCK_SIZE(*fl_idx);
        unsigned num_pages = blck_size >> SPLA_PAGE_SHIFT;
        DBG_FN1(request_pages, ("%u", num_pages));
        return spla_alloc->palloc_f(spla_alloc->pallocator,
                                    blck_size >> SPLA_PAGE_SHIFT);
    }

    spla_block **block = &spla_alloc->free_blocks[*fl_idx];
    if (*block == NULL) {
        *fl_idx += 1;
        return spla_pop_free_block(spla_alloc, fl_idx);
    } else {
        DBG_FN1(pop_free_block, ("0x%lx", FL_IDX_TO_BLCK_SIZE(*fl_idx)));

        SPLA_LOCK_ATOMIC;
        spla_block *popped_block = *block;
        *block = (*block)->next;
        spla_check(spla_alloc);
        SPLA_UNLOCK_ATOMIC;
        
        return popped_block;
    }
}

static void *spla_malloc_area(splinter_alloc *spla_alloc, size_t *size) {
    assert(*size > 0);
    unsigned blck_align = GE_POW2_SHIFT(*size);
    unsigned fl_idx = BLCK_ALIGN_TO_FL_IDX(blck_align);

    void *ptr = spla_pop_free_block(spla_alloc, &fl_idx);

    *size = FL_IDX_TO_BLCK_SIZE(fl_idx);

    return ptr;
}

void *spla_malloc(splinter_alloc *spla_alloc, size_t size) {
    assert(sizeof(size_t) <= (size_t)1 << SPLA_MIN_ALIGNMENT_SHIFT);
    size = ALIGN_UP(size, SPLA_MIN_ALIGNMENT_SHIFT);

    size_t alloc_size = size + sizeof(size_t);
    void *ptr = spla_malloc_area(spla_alloc, &alloc_size);
    void *limit = ptr + alloc_size;

    *(size_t *)ptr = size;
    ptr += sizeof(size_t);

    if (ptr + size < limit) {
        spla_free_area(spla_alloc, ptr + size, limit);
    }

    return ptr;
}

void *spla_memalign(splinter_alloc *spla_alloc, size_t align, size_t size) {
    if (LE_POW2(align) != align) {
        return NULL;
    }

    assert(sizeof(size_t) <= (size_t)1 << SPLA_MIN_ALIGNMENT_SHIFT);
    size = ALIGN_UP(size, SPLA_MIN_ALIGNMENT_SHIFT);

    size_t alloc_size =
        ALIGN_UP_SIZE(sizeof(size_t), align) + ALIGN_UP_SIZE(size, align);
    void *alloc_ptr = spla_malloc_area(spla_alloc, &alloc_size);
    void *limit = alloc_ptr + alloc_size;

    void *ptr = ALIGN_UP_SIZE(alloc_ptr + sizeof(size_t), align);
    size_t *size_ptr = (size_t *)ptr - 1;
    *size_ptr = size;

    if (alloc_ptr < (void *)size_ptr) {
        spla_free_area(spla_alloc, alloc_ptr, size_ptr);
    }
    if (ptr + size < limit) {
        spla_free_area(spla_alloc, ptr + size, limit);
    }

    return ptr;
}
