#include <assert.h>
#include <splinter-alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void *palloc(void *pallocator, size_t num_pages) {
    printf("num_pages:\t%lu\n", num_pages);
    assert(num_pages == 1);
    void *ptr;
    int res = posix_memalign(&ptr, 1ul << 12, num_pages * (1ul << 12));
    assert(res == 0);
    printf("palloc ptr:\t0x%012lx\n", (size_t)ptr);
    return ptr;
}

void pfree(void *pallocator, void *ptr, size_t num_pages) {
    printf("pfree ptr:\t%012lx\n", (size_t)ptr);
    free(ptr);
}

int main(int argc, char const *argv[]) {
    splinter_alloc *spla_alloc = spla_init_alloc(NULL, palloc, pfree);

    void *ptrs[10];
    size_t alloc_idx = 0;

#define MALLOC(size)                                                           \
    if (alloc_idx == 0) {                                                      \
        spla_dump(spla_alloc);                                                 \
        printf("\n");                                                          \
    }                                                                          \
    assert(alloc_idx < sizeof(ptrs) / sizeof(ptrs[0]));                        \
    ptrs[alloc_idx] = spla_malloc(spla_alloc, size);                           \
    printf("ptr%lu:\t\t0x%012lx\n\n", alloc_idx + 1, (size_t)ptrs[alloc_idx]); \
    spla_dump(spla_alloc);                                                     \
    printf("\n");                                                              \
    alloc_idx++

#define FREE_ALL()                                                             \
    for (size_t i = 0; i < alloc_idx; i++) {                                   \
        printf("free ptr%lu\n", i + 1);                                        \
        spla_free(spla_alloc, ptrs[i]);                                        \
        printf("\n");                                                          \
        spla_dump(spla_alloc);                                                 \
        printf("\n");                                                          \
    }

    MALLOC(123);
    MALLOC(123);
    MALLOC(123);
    MALLOC(1234);
    MALLOC(1234);
    // MALLOC(1234);
    // MALLOC(1234);
    // MALLOC(1234);

    FREE_ALL();

    return 0;
}
