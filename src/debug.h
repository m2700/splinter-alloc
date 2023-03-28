#pragma once

#include "config.h"

#define UNUSED(x) ({ __attribute__((unused)) typeof(x) unused = x; })

#if SPLA_DEBUG || SPLA_TESTING
#include <stdio.h>
#include <stdlib.h>
#endif // SPLA_DEBUG || SPLA_TESTING

#if SPLA_DEBUG

#define DBG(f, x) fprintf(stderr, #x ":\t" f "\n", x)
#define DBG_FN1(fn, x1)                                                                            \
    fprintf(stderr, #fn ":\n  ");                                                                  \
    DBG x1
#define DBG_FN2(fn, x1, x2)                                                                        \
    DBG_FN1(fn, x1);                                                                               \
    fprintf(stderr, "  ");                                                                         \
    DBG x2;

#else // SPLA_DEBUG

#define DBG(f, x)                                                                                  \
    UNUSED(f);                                                                                     \
    UNUSED(x)
#define DBG_FN1(fn, x1) DBG x1
#define DBG_FN2(fn, x1, x2)                                                                        \
    DBG x1;                                                                                        \
    DBG x2

#endif // SPLA_DEBUG

#if SPLA_TESTING

#define DBG_ERR2(e, fn, x1, x2)                                                                    \
    fprintf(stderr, "\e[0;91merror: \e[0m");                                                       \
    DBG_FN2(fn, x1, x2);                                                                           \
    exit(e)

#endif // SPLA_TESTING