#include <stdio.h>
#include <stdlib.h>

#include "mem.h"


#define PRINT_MEM 1


static uint64_t allocated;

typedef struct { void* mem; uint64_t s; char *what;} info_t;

static info_t info[100];


void* mem_calloc(size_t size, uint64_t elems, char *what){
  uint64_t s = size * elems;
  allocated += s;
  if(PRINT_MEM) printf("mem: allocating %ld B as %ld elems of %ld B (for a total of %ld B) as %s\n", s, elems, size, allocated, what);

  void *ret = calloc(size, elems);
  if(ret == NULL) FAIL("mem: couldn't allocate %s\n", what)

  for(int i = 0; i < 100; i++)
    if(info[i].mem == NULL){
      info[i].mem = ret;
      info[i].s = s;
      info[i].what = what;
      return ret;
    }
  FAIL("mem_callc: no space left in the info!")
}

void mem_free(void* mem){
  for(int i = 0; i < 100; i++)
    if(info[i].mem == mem){
      allocated -= info[i].s;
      if(PRINT_MEM) printf("mem: freeing %ld B (for a total of %ld B) as %s\n", info[i].s, allocated, info[i].what);
      info[i].mem = NULL;
      free(mem);
      return;
    }
  FAIL("mem_free: mem info not found!")
}
