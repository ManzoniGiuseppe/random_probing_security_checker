//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <string.h>
#include "multithread.h"

#include "types.h"
#include "hashMap.h"
#include "bitArray.h"
#include "hash.h"
#include "mem.h"


//#define HASHMAP_INITIAL_BITS
//#define HASHMAP_CONTIGUOUS_BITS
//#define HASHMAP_HASH_ATTEMPTS
//#define HASHMAP_SAVE_HASH_RATIO


#define DBG_FILE "hashMap"
#define DBG_LVL DBG_HASHMAP




typedef struct{
  void* table;
  hash_compact_t *hashes; // either this
  bitArray_t isPresent;   // or this
  pow2size_t num_elem;
  uint32_t size_k;
  uint32_t size_v;
  #if IS_DBG(DBG_LVL_TOFIX)
    multithread_fu64_t hash_requests;
    multithread_fu64_t hash_lookups;
  #endif
  size_t num_curr_elem;
  const char *of_what;
} hashMap_priv_t;
#define P(pub)   ((hashMap_priv_t *) ((hashMap_t)(pub)).hashMap)


#define REDUCE_HASH(ht, hash)   ((hash).v & (P(ht)->num_elem - 1))


//--- basic info

pow2size_t hashMap_getNumElem(hashMap_t ht){
  DBG(DBG_LVL_DETAILED, "%s: getNumElem: %d\n", P(ht)->of_what, P(ht)->num_elem);
  return P(ht)->num_elem;
}

size_t hashMap_getNumCurrElem(hashMap_t ht){
  DBG(DBG_LVL_DETAILED, "%s: getNumCurrElem: %d elems\n", P(ht)->of_what, P(ht)->num_curr_elem);
  return P(ht)->num_curr_elem;
}

double hashMap_dbg_fill(hashMap_t ht){
  return P(ht)->num_curr_elem / (double) P(ht)->num_elem;
}
double hashMap_dbg_hashConflictRate(__attribute__((unused)) hashMap_t ht){
  #if IS_DBG(DBG_LVL_TOFIX)
    uint_fast64_t hash_requests = multithread_fu64_get(&P(ht)->hash_requests, MULTITHREAD_SYNC_VAL);
    uint_fast64_t hash_lookups = multithread_fu64_get(&P(ht)->hash_lookups, MULTITHREAD_SYNC_VAL);
    return (hash_lookups - hash_requests) / (double) hash_requests;
  #else
    FAIL("hashMap: Trying to get a dbg information when dbg is disabled!\n")
  #endif
}

//---- new/delete


T__THREAD_SAFE hashMap_t hashMap_new__i(bool saveHash, pow2size_t num_elem, size_t size_k, size_t size_v, T__ACQUIRES const char *ofWhat){
  hashMap_t ret = { mem_calloc(sizeof(hashMap_priv_t), 1, ofWhat) };
  void *table = mem_calloc(size_k + size_v, num_elem, ofWhat);

  P(ret)->table = table;
  P(ret)->num_elem = num_elem;
  P(ret)->size_k = size_k;
  P(ret)->size_v = size_v;
  P(ret)->num_curr_elem = 0;
  P(ret)->hashes = saveHash ? mem_calloc(sizeof(hash_compact_t), num_elem, ofWhat) : NULL;
  P(ret)->isPresent = saveHash ? NULL : BITARRAY_CALLOC(num_elem, ofWhat);
  P(ret)->of_what = ofWhat;

  #if IS_DBG(DBG_LVL_TOFIX)
    multithread_fu64_init(& P(ret)->hash_requests, 0);
    multithread_fu64_init(& P(ret)->hash_lookups, 0);
  #endif
  return ret;
}

T__THREAD_SAFE hashMap_t hashMap_new(size_t size_k, size_t size_v, T__ACQUIRES const char *of_what){
  pow2size_t num_elem = 1ll << HASHMAP_INITIAL_BITS;
  bool saveHash = sizeof(hash_compact_t) * HASHMAP_SAVE_HASH_RATIO <= size_k + size_v;     //   sizeof(hash_compact_t) / (size_k + size_v) <= 1 / HASHMAP_SAVE_HASH_RATIO

  DBG(DBG_LVL_MINIMAL, "%s: new with num_elem=%ld size_k=%ld size_v=%ld, and %s hash\n", of_what, num_elem, (long) size_k, (long) size_v, saveHash ? "with" : "without");

  return hashMap_new__i(saveHash, num_elem, size_k, size_v, of_what);
}

void hashMap_delete(hashMap_t ht){
  if(P(ht)->hashes) mem_free(P(ht)->hashes);
  if(P(ht)->isPresent) mem_free(P(ht)->isPresent);
  mem_free(P(ht)->table);
  mem_free(P(ht));
}


//---- find

static inline bool isUsedR(hashMap_t ht, reducedHash_t r){
  if(P(ht)->hashes) return P(ht)->hashes[r].v != 0;
  else return bitArray_get(P(ht)->num_elem, P(ht)->isPresent, r);
}

T__THREAD_SAFE static bool hashMap_find(hashMap_t ht, void *key, hash_t *ret, bool stopAtFirstEmpty){ // set hash if found, set first empty if present, illegal 0 otherwise
  #if IS_DBG(DBG_LVL_TOFIX)
    multithread_fu64_fetch_add(&P(ht)->hash_requests, 1, MULTITHREAD_SYNC_VAL);
  #endif

  size_t size_tot = P(ht)->size_k + P(ht)->size_v;
  *ret = (hash_t){ 0 }; // illegal return for fail

  for(int counter = 0; counter < HASHMAP_HASH_ATTEMPTS; counter++){
    #if IS_DBG(DBG_LVL_TOFIX)
      multithread_fu64_fetch_add(&P(ht)->hash_lookups, 1, MULTITHREAD_SYNC_VAL);
    #endif
    hash_t base = hash_calculate(P(ht)->size_k, key, counter);
    base.v &= ~(uint_fast64_t) MASK_OF(HASHMAP_CONTIGUOUS_BITS);

    for(int i = 0; i < 1ll << HASHMAP_CONTIGUOUS_BITS; i++){
      hash_t it = base;
      it.v |= i;
      reducedHash_t usedIt = REDUCE_HASH(ht, it);

      if(usedIt == 0) // ignore illegal.
        continue;

      if(!isUsedR(ht, usedIt)){
        if(ret->v == 0)
          *ret = it;
        if(stopAtFirstEmpty) return 0;
        else continue;
      }

      if(!P(ht)->hashes || hash_eq(P(ht)->hashes[usedIt], it)){
        void* elem = P(ht)->table + usedIt * size_tot;
        if(memcmp(elem, key, P(ht)->size_k) == 0){ // if matches
          *ret = it;
          return 1;
        }
      }
    }
  }

  return 0;
}

//---- get values


bool hashMap_contains(hashMap_t ht, void *key){
  hash_t v;
  return hashMap_find(ht, key, &v, 0);
}

hash_t hashMap_getPos(hashMap_t ht, void *key){
  hash_t hash_elem;
  if(!hashMap_find(ht, key, &hash_elem, 0)) FAIL("hashMap: %s: missing key!\n", P(ht)->of_what)
  return hash_elem;
}

bool hashMap_hasVal(hashMap_t ht, reducedHash_t reducedI, void **key, void **value){
  if(reducedI == 0 || !isUsedR(ht, reducedI))
    return 0;

  size_t size_tot = P(ht)->size_k + P(ht)->size_v;
  *key = P(ht)->table + reducedI * size_tot;
  *value = *key + P(ht)->size_k;
  return 1;
}

void* hashMap_getKeyR(hashMap_t ht, reducedHash_t pos){
  ON_DBG(DBG_LVL_TOFIX, {
    if(pos == 0 || !isUsedR(ht, pos)) FAIL("hashMap: %s: trying to get the key of a missing position (0x%llX)!\n", P(ht)->of_what, (long long) pos)
  })

  size_t size_tot = P(ht)->size_k + P(ht)->size_v;
  void *key = P(ht)->table + pos * size_tot;

  return key;
}

void* hashMap_getValueR(hashMap_t ht, reducedHash_t pos){
  return hashMap_getKeyR(ht, pos) + P(ht)->size_k;
}

static bool containsHashFast(hashMap_t ht, hash_t pos){
  reducedHash_t usedPos = REDUCE_HASH(ht, pos);

  if(P(ht)->hashes) return hash_eq(pos, P(ht)->hashes[usedPos]);
  else return bitArray_get(P(ht)->num_elem, P(ht)->isPresent, usedPos);
}

void* hashMap_getKey(hashMap_t ht, hash_t pos){
  reducedHash_t usedPos = REDUCE_HASH(ht, pos);

  ON_DBG(DBG_LVL_TOFIX, {
    if(pos.v == 0 || !containsHashFast(ht, pos)) FAIL("hashMap: %s: trying to get the key of a missing position (0x%llX)!\n", P(ht)->of_what, (long long) pos.v)
  })

  size_t size_tot = P(ht)->size_k + P(ht)->size_v;
  void *key = P(ht)->table + usedPos * size_tot;

  return key;
}

void* hashMap_getValue(hashMap_t ht, hash_t pos){
  return hashMap_getKey(ht, pos) + P(ht)->size_k;
}

//---- set

static hash_t getHashOfPresent(hashMap_t ht, reducedHash_t r){
  if(P(ht)->hashes) return hash_fromCompact(P(ht)->hashes[r]);
  hash_t ret;
  if(!hashMap_find(ht, hashMap_getKeyR(ht, r), &ret, 0)) FAIL("%s: getHashOfPresent: couldn't find hash 0x%llX which should be present", P(ht)->of_what, (long long) r)
  return ret;
}

static void resize(hashMap_t ht){
  ON_DBG(DBG_LVL_TOFIX, {
    if(P(ht)->num_curr_elem < P(ht)->num_elem / 2) DBG(DBG_LVL_TOFIX, "%s: resize. Contains %ld elems out of %ld. Fill=%d%%, HashConflictRate=%d%%. Now doubling the size.\n", P(ht)->of_what, P(ht)->num_curr_elem, P(ht)->num_elem, (int) (hashMap_dbg_fill(ht) * 100), (int) (hashMap_dbg_hashConflictRate(ht) * 100));
  })

  DBG(DBG_LVL_DETAILED, "%s: risizing with num_elem=%ld, size_k=%ld size_v=%ld\n", P(ht)->of_what, (long) P(ht)->num_elem << 1, (long) P(ht)->size_k, (long) P(ht)->size_v);
  hashMap_t newMap = hashMap_new__i(P(ht)->hashes != NULL, P(ht)->num_elem << 1, P(ht)->size_k, P(ht)->size_v, P(ht)->of_what);
  size_t size_tot = P(ht)->size_k + P(ht)->size_v;

  HASHMAP_ITERATE(ht, usedHash, key, value, {
    DBG(DBG_LVL_MAX, "%s: considering element with usedHash=%llX\n", P(ht)->of_what, (long long) usedHash);
    hash_t hash = getHashOfPresent(ht, usedHash);
    DBG(DBG_LVL_MAX, "%s: usedHash=%llX corresponds to full hash=%llX\n", P(ht)->of_what, (long long) usedHash, (long long) hash.v);

    reducedHash_t newUsedHash = REDUCE_HASH(newMap, hash);
    void *newKey = P(newMap)->table + newUsedHash * size_tot;
    void *newValue = newKey + P(ht)->size_k;

    memcpy(newKey, key, P(ht)->size_k);
    memcpy(newValue, value, P(ht)->size_v);

    if(P(newMap)->hashes) P(newMap)->hashes[newUsedHash] = hash_toCompact(hash);
    else bitArray_set(P(newMap)->num_elem, P(newMap)->isPresent, newUsedHash);
  })
  SWAP(void *, P(newMap)->table, P(ht)->table)
  SWAP(hash_compact_t *, P(newMap)->hashes, P(ht)->hashes)
  SWAP(bitArray_t, P(newMap)->isPresent, P(ht)->isPresent)
  SWAP(pow2size_t, P(newMap)->num_elem, P(ht)->num_elem)
  hashMap_delete(newMap);
}

hash_t hashMap_set(hashMap_t ht, void *key, void *value){
  DBG(DBG_LVL_DETAILED, "%s: set key value pair, with numElem=%d size_k=%d size_v=%d\n", P(ht)->of_what, P(ht)->num_elem, P(ht)->size_k, P(ht)->size_v);

  size_t size_tot = P(ht)->size_k + P(ht)->size_v;
  void *mapKey;
  void *mapValue;

  hash_t hash;
  reducedHash_t usedHash;
  if(hashMap_find(ht, key, &hash, 0)){ // key present, only add value
    usedHash = REDUCE_HASH(ht, hash);
    DBG(DBG_LVL_MAX, "%s: present key, hash=0x%llX, usedHash=0x%llX\n", P(ht)->of_what, (long long) hash.v, (long long) usedHash);

    mapKey = P(ht)->table + usedHash * size_tot;
    mapValue = mapKey + P(ht)->size_k;

    memcpy(mapValue, value, P(ht)->size_v);
  }else{
    while(hash.v == 0){ // while illegal
      resize(ht);
      hashMap_find(ht, key, &hash, 1);
    }

    usedHash = REDUCE_HASH(ht, hash);
    DBG(DBG_LVL_MAX, "%s: new key, hash=0x%llX, usedHash=0x%llX\n", P(ht)->of_what, (long long) hash.v, (long long) usedHash);

    P(ht)->num_curr_elem++;
    if(P(ht)->hashes) P(ht)->hashes[usedHash] = hash_toCompact(hash);
    else bitArray_set(P(ht)->num_elem, P(ht)->isPresent, usedHash);

    mapKey = P(ht)->table + usedHash * size_tot;
    mapValue = mapKey + P(ht)->size_k;

    memcpy(mapValue, value, P(ht)->size_v);
    memcpy(mapKey, key, P(ht)->size_k);
  }


  ON_DBG(DBG_LVL_TOFIX, {
    if(usedHash == 0) FAIL("%s: set: usedHash == 0, which is illegal\n", P(ht)->of_what)
    if(!hash_eq(hashMap_getPos(ht, key), hash)) FAIL("%s: set: key's position can't be obtained from the key\n", P(ht)->of_what)
    if(hashMap_getKey(ht, hash) != mapKey) FAIL("%s: set: key has the wrong ptr in the table\n", P(ht)->of_what)
    if(hashMap_getValue(ht, hash) != mapValue) FAIL("%s: set: value has the wrong ptr in the table\n", P(ht)->of_what)
  })

  return hash;
}
