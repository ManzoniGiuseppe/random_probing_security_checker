//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include "types.h"
#include "hash.h"
#include <stddef.h>

typedef struct { void * hashMap; } hashMap_t;
#if HASH_WIDTH <= 32
  typedef uint_fast32_t reducedHash_t;
#else
  #error "unsupported HASH_WIDTH > 32"
#endif

// hash_t with the current used bit = 0 is illegal. this means an all 0s index is always illegal.

T__THREAD_SAFE hashMap_t hashMap_new(size_t size_k, size_t size_v, T__ACQUIRES const char *of_what);
void hashMap_delete(hashMap_t ht);

hash_t hashMap_set(hashMap_t ht, void *key, void *value);

T__THREAD_SAFE bool hashMap_contains(hashMap_t ht, void *key);

// returns the hash. key must have been added already
T__THREAD_SAFE hash_t hashMap_getPos(hashMap_t ht, void* key);

// returns the current hash. key must have been added already
T__THREAD_SAFE T__LEAKS void* hashMap_getKey(hashMap_t ht, hash_t pos);
T__THREAD_SAFE T__LEAKS void* hashMap_getKeyR(hashMap_t ht, reducedHash_t pos);

// returns the current hash. key must have been added already
// this is mutable
T__THREAD_SAFE T__LEAKS void* hashMap_getValue(hashMap_t ht, hash_t pos);
T__THREAD_SAFE T__LEAKS void* hashMap_getValueR(hashMap_t ht, reducedHash_t pos);

T__THREAD_SAFE pow2size_t hashMap_getNumElem(hashMap_t ht);
T__THREAD_SAFE size_t hashMap_getNumCurrElem(hashMap_t ht);


T__THREAD_SAFE double hashMap_dbg_fill(hashMap_t ht);
T__THREAD_SAFE double hashMap_dbg_hashConflictRate(hashMap_t ht);



T__THREAD_SAFE T__LEAKS bool hashMap_hasVal(hashMap_t ht, reducedHash_t reducedI, void **ret_key, void **ret_value);

// REDUCED_INDEX is an index that is valid insofar as th map doesn't change size.
#define HASHMAP_ITERATE(map, REDUCED_INDEX, KEY, VALUE, CODE) {\
  size_t HASHMAP_ITERATE__size = hashMap_getNumElem(map);\
  for(reducedHash_t REDUCED_INDEX = 0; REDUCED_INDEX < HASHMAP_ITERATE__size; REDUCED_INDEX++){\
    void *KEY;\
    void *VALUE;\
    if(hashMap_hasVal(map, REDUCED_INDEX, &KEY, &VALUE)){\
      { CODE }\
    }\
  }\
}


#endif // _HASH_MAP_H_
