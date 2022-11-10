#pragma once

#include "config.h"

#if SPLA_TESTING
void spla_check(splinter_alloc *spla_alloc);
#else // SPLA_TESTING
#define spla_check(spla_alloc)
#endif // SPLA_TESTING