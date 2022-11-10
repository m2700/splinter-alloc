#pragma once

#include "config.h"

#if SPLA_DEBUG || SPLA_TESTING
#include <stdio.h>
#include <stdlib.h>
#endif // SPLA_DEBUG || SPLA_TESTING

#if SPLA_DEBUG

#define DBG(f, x) fprintf(stderr, #x ":\t" f "\n", x)
#define DBG_FN1(fn, x1)                                                        \
    fprintf(stderr, #fn ":\n  ");                                              \
    DBG x1
#define DBG_FN2(fn, x1, x2)                                                    \
    DBG_FN1(fn, x1);                                                           \
    fprintf(stderr, "  ");                                                     \
    DBG x2;

#else // SPLA_DEBUG

#define DBG(t, x)
#define DBG_FN(fn, x1)
#define DBG_FN2(fn, x1, x2)

#endif // SPLA_DEBUG

#if SPLA_TESTING

#define DBG_ERR2(e, fn, x1, x2)                                                \
    fprintf(stderr, "\e[0;91merror: \e[0m");                                   \
    DBG_FN2(fn, x1, x2);                                                       \
    exit(e)

#endif // SPLA_TESTING