#pragma once

#ifndef SPLA_AVL_FREE_LISTS
#define SPLA_AVL_FREE_LISTS 0
#endif

#ifndef SPLA_MIN_ALIGNMENT_SHIFT
#if SPLA_AVL_FREE_LISTS
#define SPLA_MIN_ALIGNMENT_SHIFT 6
#else // SPLA_AVL_FREE_LISTS
#define SPLA_MIN_ALIGNMENT_SHIFT 3
#endif // SPLA_AVL_FREE_LISTS
#endif // SPLA_MIN_ALIGNMENT_SHIFT

#if SPLA_MIN_ALIGNMENT_SHIFT < 3 || (SPLA_AVL_FREE_LISTS && SPLA_MIN_ALIGNMENT_SHIFT < 6)
#error "spla_block does not fit in the smallest free block"
#endif

#ifndef SPLA_MAX_ALIGNMENT_SHIFT
#define SPLA_MAX_ALIGNMENT_SHIFT 11
#endif

#if SPLA_MAX_ALIGNMENT_SHIFT < SPLA_MIN_ALIGNMENT_SHIFT
#error "max alignment is smaller than minimum"
#endif

#define SPLA_PAGE_SHIFT (SPLA_MAX_ALIGNMENT_SHIFT + 1)
#define SPLA_PAGE_SIZE ((size_t)1 << SPLA_PAGE_SHIFT)

#define SPLA_MIN_BLOCK_SIZE (1 << SPLA_MIN_ALIGNMENT_SHIFT)

#ifndef SPLA_DEBUG
#define SPLA_DEBUG 0
#endif

#ifndef SPLA_TESTING
#define SPLA_TESTING 0
#endif

#ifndef SPLA_SINGLE_CPU
#define SPLA_SINGLE_CPU 0
#endif

#ifndef SPLA_LOCK_ATOMIC
#if SPLA_SINGLE_CPU
#define SPLA_LOCK_ATOMIC asm("cli")
#else // SPLA_SINGLE_CPU
#define SPLA_LOCK_ATOMIC
#endif // SPLA_SINGLE_CPU
#endif // SPLA_LOCK_ATOMIC

#ifndef SPLA_UNLOCK_ATOMIC
#if SPLA_SINGLE_CPU
#define SPLA_UNLOCK_ATOMIC asm("sti")
#else // SPLA_SINGLE_CPU
#define SPLA_UNLOCK_ATOMIC
#endif // SPLA_SINGLE_CPU
#endif

#ifndef SPLA_ALLOCATE_EXACT
#define SPLA_ALLOCATE_EXACT 0
#endif

#ifndef SPLA_SORT_COMPACT_TRIES
#define SPLA_SORT_COMPACT_TRIES -1
#endif

#if SPLA_CONFIG_EXTENTION
#include <splinter-alloc-config-ext.h>
#endif
