#pragma once

#include <stddef.h>

typedef void *(*spla_palloc_func_t)(void *pallocator, size_t num_pages);
typedef void (*spla_pfree_func_t)(void *pallocator, void *ptr,
                                  size_t num_pages);

typedef struct splinter_alloc splinter_alloc;

splinter_alloc *spla_init_alloc(void *pallocator, spla_palloc_func_t palloc_f,
                                spla_pfree_func_t pfree_f);

void *spla_malloc(splinter_alloc *spla_alloc, size_t size);
void *spla_memalign(splinter_alloc *spla_alloc, size_t align, size_t size);

void *spla_realloc(splinter_alloc *spla_alloc, void *ptr, size_t size);

void spla_free(splinter_alloc *spla_alloc, void *ptr);

void spla_dump(splinter_alloc *spla_alloc);
