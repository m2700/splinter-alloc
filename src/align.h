#pragma once

#include "bitidx.h"

#define SHIFT_TO_REST_MASK(shift) (((size_t)1 << (shift)) - 1)
#define SHIFT_TO_MASK(shift) ~SHIFT_TO_REST_MASK(shift)
#define ALIGN_UP(x, shift)                                                                         \
    (typeof(x))(((size_t)(x) + SHIFT_TO_REST_MASK(shift)) & SHIFT_TO_MASK(shift))
#define ALIGN_UP_SIZE(x, align_size)                                                               \
    (typeof(x))(((size_t)(x) + ((align_size)-1)) & ~((align_size)-1))
#define DIV_SHIFT_CEIL(x, shift) ((((x) + SHIFT_TO_REST_MASK(shift)) >> (shift)))

/// x must not be 0
#define GE_POW2_SHIFT(x) ((x) == 1 ? 0 : sizeof(long) * 8 - clz((long)(x)-1))
/// returns 0 for x = 0
#define GE_POW2(x) ((x) == 0 ? 0 : (size_t)1 << GE_POW2_SHIFT(x))

/// x must not be 0
#define LE_POW2_SHIFT(x) (sizeof(long) * 8 - clz((long)(x)) - 1)
/// returns 0 for x = 0
#define LE_POW2(x) ((x) == 0 ? 0 : (size_t)1 << LE_POW2_SHIFT(x))

#define MAX_ALIGN_OF(x) ((x) == 0 ? sizeof(size_t) * 8 - 1 : ctz((long)(x)))