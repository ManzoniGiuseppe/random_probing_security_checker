//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <string.h>

#include "hashCache.h"
#include "mem.h"
#include "hash.h"


#define DBG_FILE  "hashCache"
#define DBG_LVL  DBG_HASHCACHE


// #define HASHCACHE_BITS
// #define HASHCACHE_WAYS


#define SIZE  (1ll << HASHCACHE_BITS)

struct valInfo{
  uint_least8_t next;
  uint_least8_t size;
};

typedef struct {
  size_t size_k;
  size_t size_v;
  T__ACQUIRES const char *ofWhat;
  void *table;
  struct valInfo *info;
} hashCache_state_t;
#define P(pub)   ((hashCache_state_t *) ((hashCache_t)(pub)).hashCache)


T__THREAD_SAFE hashCache_t hashCache_new(size_t size_k, size_t size_v, T__ACQUIRES const char *ofWhat){
  hashCache_t ret = { mem_calloc(sizeof(hashCache_state_t), 1, ofWhat) };
  P(ret)->size_k = size_k;
  P(ret)->size_v = size_v;
  P(ret)->ofWhat = ofWhat;
  P(ret)->table = mem_calloc(size_k + size_v, SIZE * HASHCACHE_WAYS, ofWhat);
  P(ret)->info = mem_calloc(sizeof(struct valInfo), SIZE, ofWhat);

  size_t last = (P(ret)->size_k + P(ret)->size_v) * SIZE * HASHCACHE_WAYS - 1;
  DBG(DBG_LVL_MAX, "last=0x%llX\n", (long long) last);

  return ret;
}


void hashCache_delete(hashCache_t s){
  mem_free(P(s)->table);
  mem_free(P(s)->info);
  mem_free(P(s));
}


void hashCache_set(hashCache_t s, void *key, void *value){
  DBG(DBG_LVL_DETAILED, "%s: set value\n", P(s)->ofWhat)
  hash_t h = hash_calculate(P(s)->size_k, key, 0);
  size_t size_tot = P(s)->size_k + P(s)->size_v;
  size_t index = h.v & MASK_OF(HASHCACHE_BITS);

  struct valInfo *info = & P(s)->info[index];
  void *pos = P(s)->table + size_tot * index * HASHCACHE_WAYS;

  DBG(DBG_LVL_MAX, "index 0x%llX, size 0x%llX\n", (long long) index, (long long) SIZE)
  for(int i = 0; i < info->size; i++){
    DBG(DBG_LVL_MAX, "checking internal index %d of %d\n", i, info->size)
    if(memcmp(pos + size_tot * i, key, P(s)->size_k) == 0){
      DBG(DBG_LVL_MAX, "already present\n")
      return;
    }
  }
  DBG(DBG_LVL_MAX, "not present. next=%d, size=%d, size_tot=%lld\n", (int) info->next, (int) info->size, (long long) size_tot)

  memcpy(pos + size_tot * info->next               , key,   P(s)->size_k);
  DBG(DBG_LVL_MAX, "key set\n")
  memcpy(pos + size_tot * info->next + P(s)->size_k, value, P(s)->size_v);
  DBG(DBG_LVL_MAX, "value set\n")
  info->next = (info->next+1) % HASHCACHE_WAYS;
  info->size = MIN(info->size+1, HASHCACHE_WAYS);

  DBG(DBG_LVL_MAX, "added\n")
}

T__THREAD_SAFE T__LEAKS void* hashCache_getValue(hashCache_t s, void *key){ // returns NULL if missing
  DBG(DBG_LVL_DETAILED, "%s: getValue\n", P(s)->ofWhat)
  hash_t h = hash_calculate(P(s)->size_k, key, 0);
  size_t size_tot = P(s)->size_k + P(s)->size_v;
  size_t index = h.v & MASK_OF(HASHCACHE_BITS);

  struct valInfo *info = & P(s)->info[index];
  void *pos = P(s)->table + size_tot * index * HASHCACHE_WAYS;

  DBG(DBG_LVL_MAX, "index 0x%llX, size 0x%llX\n", (long long) index, (long long) SIZE)
  for(int i = 0; i < info->size; i++){
    DBG(DBG_LVL_MAX, "checking internal index %d of %d\n", i, info->size)
    if(memcmp(pos + size_tot * i, key, P(s)->size_k) == 0){
      DBG(DBG_LVL_MAX, "found\n")
      return pos + size_tot * i + P(s)->size_k;
    }
  }

  DBG(DBG_LVL_MAX, "missing\n")
  return NULL;
}
