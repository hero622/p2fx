#pragma once

#include "Memalloc.hpp"

#define free(p) g_pMemAlloc->Free(p)
#define malloc(s) MemAlloc_Alloc(s, __FILE__, __LINE__)
#define realloc(p, s) g_pMemAlloc->Realloc(p, s, __FILE__, __LINE__)
#define _aligned_malloc(s, a) MemAlloc_AllocAlignedFileLine(s, a, __FILE__, __LINE__)