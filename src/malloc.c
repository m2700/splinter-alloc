#include <assert.h>
#include <splinter-alloc.h>

#include "align.h"
#include "alloc.h"
#include "check.h"
#include "config.h"
#include "debug.h"
#include "free.h"

static void *spla_pop_free_block(splinter_alloc *spla_alloc, unsigned *fl_idx) {
    if (*fl_idx >= SPLA_NUM_BLOCK_SIZES) {
        return NULL;
    }

    spla_block **block = &spla_alloc->free_blocks[*fl_idx];
    if (*block == NULL) {
        *fl_idx += 1;
        return spla_pop_free_block(spla_alloc, fl_idx);
    } else {
        DBG_FN1(pop_free_block, ("0x%lx", FL_IDX_TO_BLCK_SIZE(*fl_idx)));

        SPLA_LOCK_ATOMIC;

#if SPLA_AVL_FREE_LISTS
        spla_avl_node *popped_block = spla_avl_tree_pop(block);
#else  // SPLA_AVL_FREE_LISTS
        spla_block *popped_block = *block;
        *block = (*block)->next;
#endif // SPLA_AVL_FREE_LISTS

        spla_check(spla_alloc);

        SPLA_UNLOCK_ATOMIC;

        return popped_block;
    }
}

static void *spla_malloc_area(splinter_alloc *spla_alloc, size_t *size) {
    assert(*size > 0);
    assert(*size >= SPLA_MIN_BLOCK_SIZE);

    unsigned blck_align = GE_POW2_SHIFT(*size);
    unsigned fl_idx = BLCK_ALIGN_TO_FL_IDX(blck_align);

    void *ptr = spla_pop_free_block(spla_alloc, &fl_idx);

    if (ptr == NULL) {
        assert(fl_idx >= SPLA_NUM_BLOCK_SIZES);
        *size = ALIGN_UP(*size, SPLA_PAGE_SHIFT);
        unsigned num_pages = *size >> SPLA_PAGE_SHIFT;
        DBG_FN1(request_pages, ("%u", num_pages));
        return spla_alloc->palloc_f(spla_alloc->pallocator, num_pages);
    } else {
        *size = FL_IDX_TO_BLCK_SIZE(fl_idx);
    }

    return ptr;
}

void *spla_malloc(splinter_alloc *spla_alloc, size_t size) {
    if (size == 0) {
        return NULL;
    }

    assert(sizeof(size_t) <= (size_t)1 << SPLA_MIN_ALIGNMENT_SHIFT);

    const size_t min_alloc_size = ALIGN_UP(size + sizeof(size_t), SPLA_MIN_ALIGNMENT_SHIFT);
    size = min_alloc_size - sizeof(size_t);

    size_t alloc_size = min_alloc_size;
    void *ptr = spla_malloc_area(spla_alloc, &alloc_size);
    if (ptr == NULL) {
        return NULL;
    }
    void *limit = ptr + alloc_size;

#if !SPLA_ALLOCATE_EXACT
    size_t min_alloc_size_2p = GE_POW2(min_alloc_size);
    if (min_alloc_size_2p <= alloc_size) {
        size = min_alloc_size_2p - sizeof(size_t);
    }
#endif

    *(size_t *)ptr = size;
    ptr += sizeof(size_t);

    if (ptr + size < limit) {
        spla_free_area(spla_alloc, ptr + size, limit);
    }

    return ptr;
}

void *spla_memalign(splinter_alloc *spla_alloc, size_t align, size_t size) {
    if (size == 0) {
        return NULL;
    }
    if (LE_POW2(align) != align) {
        return NULL;
    }

    assert(sizeof(size_t) <= (size_t)1 << SPLA_MIN_ALIGNMENT_SHIFT);

    size_t alloc_size = ALIGN_UP_SIZE(sizeof(size_t), align) + ALIGN_UP_SIZE(size, align);
    alloc_size = ALIGN_UP(alloc_size, SPLA_MIN_ALIGNMENT_SHIFT);

    void *alloc_ptr = spla_malloc_area(spla_alloc, &alloc_size);
    if (alloc_ptr == NULL) {
        return NULL;
    }
    void *limit = alloc_ptr + alloc_size;

    void *ptr = ALIGN_UP_SIZE(alloc_ptr + sizeof(size_t), align);
    size_t *size_ptr = (size_t *)ptr - 1;

    void *min_limit = ALIGN_UP(ptr + size, SPLA_MIN_ALIGNMENT_SHIFT);

    size = min_limit - ptr;

#if !SPLA_ALLOCATE_EXACT
    const size_t min_alloc_size_2p = GE_POW2((size_t)(min_limit - alloc_ptr));
    if (min_alloc_size_2p <= alloc_size) {
        size = min_alloc_size_2p - (ptr - alloc_ptr);
    }
#endif

    *size_ptr = size;

    void *align_down_size_ptr = ALIGN_DOWN((void *)size_ptr, SPLA_MIN_ALIGNMENT_SHIFT);

    if (alloc_ptr < align_down_size_ptr) {
        spla_free_area(spla_alloc, alloc_ptr, align_down_size_ptr);
    }
    if (ptr + size < limit) {
        spla_free_area(spla_alloc, ptr + size, limit);
    }

    return ptr;
}
