#pragma once

#include <splinter-alloc.h>
#include <stddef.h>

void spla_alloc_free_fifo(splinter_alloc *spla_alloc, size_t num_allocs, size_t block_size);
void spla_alloc_free_lifo(splinter_alloc *spla_alloc, size_t num_allocs, size_t block_size);
void spla_alloc_free_random(splinter_alloc *spla_alloc, size_t num_allocs, size_t block_size);