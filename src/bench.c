#include <splinter-alloc/bench.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NS_PER_SEC 1000000000
#define TIMESPEC_DIFF_NS(tp_s, tp_e)                                                               \
    ((tp_e.tv_sec - tp_s.tv_sec) * NS_PER_SEC + tp_e.tv_nsec - tp_s.tv_nsec)
#define FMT_NS(header, ns, end) printf(header "%lu.%09lu" end, ns / NS_PER_SEC, ns % NS_PER_SEC)

void spla_alloc_free_fifo(splinter_alloc *spla_alloc, size_t num_allocs, size_t block_size) {
    printf("spla_alloc_free_fifo(num_allocs: %lu, block_size: %lu):\n", num_allocs, block_size);

    void *ptrs[num_allocs];
    struct timespec tp_s, tp_e;

    clock_gettime(CLOCK_MONOTONIC, &tp_s);
    for (size_t i = 0; i < num_allocs; i++) {
        ptrs[i] = spla_malloc(spla_alloc, block_size);
    }
    clock_gettime(CLOCK_MONOTONIC, &tp_e);
    FMT_NS("    alloc (s):", TIMESPEC_DIFF_NS(tp_s, tp_e), "\n");

    clock_gettime(CLOCK_MONOTONIC, &tp_s);
    for (size_t i = 0; i < num_allocs; i++) {
        spla_free(spla_alloc, ptrs[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &tp_e);
    FMT_NS("    free (s):", TIMESPEC_DIFF_NS(tp_s, tp_e), "\n");
}

void spla_alloc_free_lifo(splinter_alloc *spla_alloc, size_t num_allocs, size_t block_size) {
    printf("spla_alloc_free_lifo(num_allocs: %lu, block_size: %lu):\n", num_allocs, block_size);

    void *ptrs[num_allocs];
    struct timespec tp_s, tp_e;

    clock_gettime(CLOCK_MONOTONIC, &tp_s);
    for (size_t i = 0; i < num_allocs; i++) {
        ptrs[i] = spla_malloc(spla_alloc, block_size);
    }
    clock_gettime(CLOCK_MONOTONIC, &tp_e);
    FMT_NS("    alloc (s):", TIMESPEC_DIFF_NS(tp_s, tp_e), "\n");

    clock_gettime(CLOCK_MONOTONIC, &tp_s);
    for (size_t i = num_allocs; i-- > 0;) {
        spla_free(spla_alloc, ptrs[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &tp_e);
    FMT_NS("    free (s):", TIMESPEC_DIFF_NS(tp_s, tp_e), "\n");
}

void spla_alloc_free_random(splinter_alloc *spla_alloc, size_t num_allocs, size_t block_size) {
    printf("spla_alloc_free_random(num_allocs: %lu, block_size: %lu):\n", num_allocs, block_size);

    void *ptrs[num_allocs];
    struct timespec tp_s, tp_e;

    clock_gettime(CLOCK_MONOTONIC, &tp_s);
    for (size_t i = 0; i < num_allocs; i++) {
        ptrs[i] = spla_malloc(spla_alloc, block_size);
    }
    clock_gettime(CLOCK_MONOTONIC, &tp_e);
    FMT_NS("    alloc (s):", TIMESPEC_DIFF_NS(tp_s, tp_e), "\n");

    for (size_t i = 0; i < num_allocs; i++) {
        size_t j = rand() % num_allocs;
        void *tmp = ptrs[i];
        ptrs[i] = ptrs[j];
        ptrs[j] = tmp;
    }

    clock_gettime(CLOCK_MONOTONIC, &tp_s);
    for (size_t i = 0; i < num_allocs; i++) {
        spla_free(spla_alloc, ptrs[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &tp_e);
    FMT_NS("    free (s):", TIMESPEC_DIFF_NS(tp_s, tp_e), "\n");
}