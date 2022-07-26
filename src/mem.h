#ifndef _MEM_H_
#define _MEM_H_

#include <stddef.h>
#include "types.h"


void* mem_calloc(size_t size, uint64_t elems, char *what);
void mem_free(void* mem);

#endif // _MEM_H_

