//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "multithread.h"


//#define MEM_NUM_ALLOCS


#define DBG_FILE "mem"
#define DBG_LVL DBG_MEM


static multithread_mtx_t mutex;
static uint64_t allocated;
static uint64_t max_allocated;

typedef struct { void* mem; uint64_t s; const char *what;} info_t;

static info_t info[MEM_NUM_ALLOCS];

static void checkConsistency(void){
  uint64_t s = 0;
  for(int i = 0; i < MEM_NUM_ALLOCS; i++)
    if(info[i].mem != NULL)
      s += info[i].s;
  if(s != allocated) FAIL("Total allocated %d, but sum of allocated is %d\n", allocated, s)
}


void* mem_calloc(size_t size, uint64_t elems, const char *what){
  if(!multithread_mtx_lock(&mutex)) FAIL("%s: mem_calloc: Error in locking the mutex.\n", what);

  ON_DBG(DBG_LVL_TOFIX, { checkConsistency(); })

  DBG(DBG_LVL_DETAILED, "allocating %ld B as %ld elems of %ld B (for a total of %ld B) as %s\n", size * elems, elems, size, MAX(max_allocated, allocated + size * elems), what);

  void *ret = calloc(size, elems);
  if(ret == NULL) FAIL("[mem] couldn't allocate %s. It'd require %ld B (for a total of %ld B)\n", what, size * elems, allocated + size * elems)

  ON_DBG(DBG_LVL_TOFIX, {
    uint64_t s = size * elems;
    allocated += s;
    max_allocated = MAX(max_allocated, allocated);

    bool found = 0;
    for(int i = 0; i < MEM_NUM_ALLOCS && !found; i++)
      if(info[i].mem == NULL){
        info[i].mem = ret;
        info[i].s = s;
        info[i].what = what;

        checkConsistency();
        found = 1;
      }
    if(!found) FAIL("mem_callc: no space left in the info!\n")
  })

  if(!multithread_mtx_unlock(&mutex)) FAIL("%s: mem_calloc: Error in unlocking the mutex.\n", what);
  return ret;
}

void mem_free(void* mem){
  if(mem == NULL) FAIL("mem_free: Trying to free the null array!\n")
  if(!multithread_mtx_lock(&mutex)) FAIL("mem_free: Error in locking the mutex.\n");

  ON_DBG(DBG_LVL_TOFIX, {
    checkConsistency();

    bool found = 0;
    for(int i = 0; i < MEM_NUM_ALLOCS && !found; i++)
      if(info[i].mem == mem){
        allocated -= info[i].s;
        DBG(DBG_LVL_DETAILED, "freeing %ld B (for a total of %ld B) as %s\n", info[i].s, allocated, info[i].what);
        info[i].mem = NULL;

        checkConsistency();
        found = 1;
      }
    if(!found) FAIL("mem_free: mem info not found!\n")
  })

  free(mem);
  if(!multithread_mtx_unlock(&mutex)) FAIL("mem_free: Error in unlocking the mutex.\n");
}

static void __attribute__ ((constructor)) mem_init(void){
  if(!multithread_mtx_init(&mutex)) FAIL("Error in initing the mem's mutex.\n");
}
static void __attribute__ ((destructor)) mem_deinit(void){
  multithread_mtx_destroy(&mutex);
  DBG(DBG_LVL_MINIMAL, "max allocated %ld B\n", max_allocated);
  ON_DBG(DBG_LVL_TOFIX, {
    if(allocated){
      DBG(DBG_LVL_TOFIX, "final balance %ld B is not 0!\n", allocated);
      for(int i = 0; i < MEM_NUM_ALLOCS; i++){
        if(info[i].mem != NULL){
          DBG(DBG_LVL_TOFIX, "  left over %s\n", info[i].what)
        }
      }
    }
  })
}
