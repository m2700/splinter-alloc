#include <assert.h>
#include <splinter-alloc.h>
#include <splinter-alloc/bench.h>
#include <stdlib.h>

void *palloc(void *pallocator, size_t num_pages) {
    void *ptr;
    int res = posix_memalign(&ptr, 1ul << 12, num_pages * (1ul << 12));
    assert(res == 0);
    return ptr;
}

void pfree(void *pallocator, void *ptr, size_t num_pages) {
    // we can't free here,
    // because we don't know if this pointer
    // was used for allocation
}

int main(int argc, char const *argv[]) {
    splinter_alloc *spla_alloc = spla_init_alloc(NULL, 0, palloc, pfree);

    spla_alloc_free_random(spla_alloc, 100000, 64);

    return 0;
}
