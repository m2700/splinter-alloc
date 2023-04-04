#include <assert.h>
#include <splinter-alloc.h>
#include <string.h>

#include "alloc.h"
#include "config.h"
#include "free.h"

void *spla_realloc(splinter_alloc *spla_alloc, void *ptr, size_t size) {
    if (ptr == NULL) {
        return spla_malloc(spla_alloc, size);
    } else if (size == 0) {
        spla_free(spla_alloc, ptr);
        return NULL;
    }

    assert(sizeof(size_t) <= (size_t)1 << SPLA_MIN_ALIGNMENT_SHIFT);
    size = ALIGN_UP(size, SPLA_MIN_ALIGNMENT_SHIFT);

    size_t *data_size = (size_t *)ptr - 1;
    void *limit = ptr + *data_size;
    if (size <= *data_size) {
#if SPLA_ALLOCATE_EXACT
        // size is min aligned
        if (size < *data_size) {
            *data_size = size;
            spla_free_area(spla_alloc, ptr + size, limit);
        }
#endif // SPLA_ALLOCATE_EXACT
        return ptr;
    }

    void *new_ptr = spla_malloc(spla_alloc, size);
    memcpy(new_ptr, ptr, *data_size);
    spla_free(spla_alloc, ptr);

    return new_ptr;
}