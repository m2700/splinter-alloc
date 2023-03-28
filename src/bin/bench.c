#include <assert.h>
#include <splinter-alloc.h>
#include <stdlib.h>
#include <splinter-alloc/bench.h>

void *palloc(void *pallocator, size_t num_pages) {
    void *ptr;
    int res = posix_memalign(&ptr, 1ul << 12, num_pages * (1ul << 12));
    assert(res == 0);
    return ptr;
}

void pfree(void *pallocator, void *ptr, size_t num_pages) { free(ptr); }

int main(int argc, char const *argv[]) {
    splinter_alloc *spla_alloc = spla_init_alloc(NULL, 0, palloc, pfree);

    spla_alloc_free_lifo(spla_alloc, 2000, 128);

    return 0;
}
