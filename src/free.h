#pragma once

#include <splinter-alloc.h>
#include <stddef.h>

void spla_free_area(splinter_alloc *spla_alloc, void *ptr, void *limit);