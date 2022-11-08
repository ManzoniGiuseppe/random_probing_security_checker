//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _HASH_SET_H_
#define _HASH_SET_H_

#include "hashMap.h"
#include "types.h"


// internal
#define SET2MAP(ht)  ((hashMap_t){ (ht).hashSet })

typedef struct { void *hashSet; } hashSet_t;


T__THREAD_SAFE static inline hashSet_t hashSet_new(size_t size_k, T__ACQUIRES const char *of_what){ return (hashSet_t){ hashMap_new(size_k, 0, of_what).hashMap }; }
static inline void hashSet_delete(hashSet_t ht){ return hashMap_delete(SET2MAP(ht)); }

static inline hash_t hashSet_add(hashSet_t ht, void *key){ int u; return hashMap_set(SET2MAP(ht), key, &u); }

T__THREAD_SAFE static inline bool hashSet_contains(hashSet_t ht, void *key){ return hashMap_contains(SET2MAP(ht), key); }
T__THREAD_SAFE static inline hash_t hashSet_getPos(hashSet_t ht, void *key){ return hashMap_getPos(SET2MAP(ht), key); }
T__THREAD_SAFE T__LEAKS static inline void* hashSet_getKey(hashSet_t ht, hash_t pos){ return hashMap_getKey(SET2MAP(ht), pos); }
T__THREAD_SAFE T__LEAKS static inline void* hashSet_getKeyR(hashSet_t ht, reducedHash_t pos){ return hashMap_getKeyR(SET2MAP(ht), pos); }
T__THREAD_SAFE static inline pow2size_t hashSet_getNumElem(hashSet_t ht){ return hashMap_getNumElem(SET2MAP(ht)); }
T__THREAD_SAFE static inline size_t hashSet_getNumCurrElem(hashSet_t ht){ return hashMap_getNumCurrElem(SET2MAP(ht)); }
T__THREAD_SAFE static inline double hashSet_dbg_fill(hashSet_t ht){ return hashMap_dbg_fill(SET2MAP(ht)); }
T__THREAD_SAFE static inline double hashSet_dbg_hashConflictRate(hashSet_t ht){ return hashMap_dbg_hashConflictRate(SET2MAP(ht)); }


T__THREAD_SAFE T__LEAKS static inline bool hashSet_hasVal(hashSet_t ht, reducedHash_t reducedI, void **ret_key){
  hashMap_t underlying = SET2MAP(ht);
  void *val;
  return hashMap_hasVal(underlying, reducedI, ret_key, &val);
}


// USED_INDEX is an index that is valid insofar as the set doesn't change size.
#define HASHSET_ITERATE(set, REDUCED_INDEX, KEY, CODE) {\
  HASHMAP_ITERATE((hashMap_t){ (set).hashSet }, REDUCED_INDEX, KEY, HASHSET_ITERATE__value, CODE)\
}

#undef SET2MAP

#endif // _HASH_SET_H_

