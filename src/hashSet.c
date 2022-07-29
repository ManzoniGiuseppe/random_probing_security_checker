#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "hashSet.h"
#include "mem.h"


double hashSet_dbg_fill(hashSet_t ht){
  return ht->items / (double) (1ll << ht->size);
}
double hashSet_dbg_hashConflictRate(hashSet_t ht){
  return (ht->hash_lookups - ht->hash_requests) / (double) ht->hash_requests;
}

hashSet_t hashSet_new(shift_t initialSize, size_t elem_size, char *of_what){
  if(elem_size % sizeof(uint64_t) != 0) FAIL("hashSet: %s: elem_size must be a multiple of 8", of_what)

  hashSet_t ret = mem_calloc(sizeof(struct hashSet), 1, of_what);
  void *table = mem_calloc(elem_size, 1ll<<initialSize, of_what);

  *ret = (struct hashSet){
    .table = table,
    .size = initialSize,
    .items = 0,
    .hash_requests = 0,
    .hash_lookups = 0,
    .elem_size = elem_size,
    .of_what = of_what,
  };
  return ret;
}

void hashSet_delete(hashSet_t ht){
  mem_free(ht->table);
  mem_free(ht);
}


// must be pow2
#define CACHE_LINE 64

#define NUM_ATTEMPT_NON_ZERO 100
#define NUM_ATTEMPT_HASH_S 100


static hash_l_t calc_hash_l__i(hashSet_t ht, void *key_p, hash_l_t counter){
  uint64_t *key = key_p;
  size_t size = ht->elem_size/sizeof(uint64_t);

  hash_l_t hash = 1;
  for(size_t i = 0; i < size; i++){
    hash_l_t it = key[i] + counter * i;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  }

  hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  return hash;
}

static hash_l_t calc_hash_l(hashSet_t ht, void* key){
  hash_l_t counter = 0;
  hash_l_t ret = 0;
  while(ret == 0){
    if(counter == NUM_ATTEMPT_NON_ZERO) return 0;
    ret = calc_hash_l__i(ht, key, counter++);
  }
  return ret;
}

static hash_s_t calc_hash_s(hashSet_t ht, hash_l_t hash, hash_l_t counter){
  if(counter < 10){
    hash = hash * counter + ROT(hash, 7);
    hash_s_t r = 0;
    for(int i = 0; i < 64 ; i+=ht->size)
      r ^= (hash >> i) & ((1ll << ht->size) -1);
    return r;
  }else{
    hash = hash * counter + ROT(hash, 7);
    hash_s_t r = 0;
    for(int i = 0; i < 64 ; i+=ht->size)
      r ^= ((hash * counter * i) >> i) & ((1ll << ht->size) -1);
    return r;
  }
}

static int hashSet_find(hashSet_t ht, void *key, hash_s_t *hash_elem){
  ht->hash_requests++;
  hash_l_t *base_hash = (hash_l_t*) key; // struct has the same pointer as its first element

  if(*base_hash == 0) *base_hash = calc_hash_l(ht, key);
  if(*base_hash == 0){
    printf("hashSet: %s: couldn't calculate a base_hash != 0\n", ht->of_what);
    return -1;
  }
  hash_s_t block = MIN((hash_s_t) 1ll<<ht->size, 1+ (hash_s_t) (CACHE_LINE-1) / ht->elem_size);

  for(hash_s_t counter = 0; counter < NUM_ATTEMPT_HASH_S; counter++){
    hash_s_t hash = calc_hash_s(ht, *base_hash, counter);

    for(hash_s_t i = 0; i < block; i++){
      *hash_elem = (hash &~(block-1)) | ((hash+i) & (block-1));
      void* elem = ht->table + *hash_elem * ht->elem_size;
      ht->hash_lookups++;

      if(*(hash_l_t*) elem == 0) // struct has the same pointer as its first element
        return 0;

      if(memcmp(elem, key, ht->elem_size) == 0)
        return 1;
    }
  }

  printf("hashSet: %s: couldn't find a match or an empty place! fill=%d%%, conflictRate=%d%%\n", ht->of_what, (int) (hashSet_dbg_fill(ht) * 100), (int) (hashSet_dbg_hashConflictRate(ht) * 100));
  return -1;
}

bool hashSet_contains(hashSet_t ht, void *key){
  hash_s_t hash_elem;
  int ret = hashSet_find(ht, key, &hash_elem);
  return ret == 1;
}

bool hashSet_add(hashSet_t ht, void *key){
  hash_s_t hash_elem;
  int ret = hashSet_find(ht, key, &hash_elem);

  if(ret == -1) return 0;
  if(ret == 1) FAIL("hashTable: %s: key already present\n", ht->of_what)

   ht->items++;
   memcpy(ht->table + hash_elem * ht->elem_size, key, ht->elem_size);
   return 1;
}

hash_s_t hashSet_getCurrentPos(hashSet_t ht, void *key){
  hash_s_t hash_elem;
  int ret = hashSet_find(ht, key, &hash_elem);

  if(ret == -1) return 0;
  if(ret == 0) FAIL("hashTable: %s: missing key\n", ht->of_what)

   return hash_elem;
}

void* hashSet_getKey(hashSet_t ht, hash_s_t pos){
  return ht->table + pos * ht->elem_size;
}

bool hashSet_validPos(hashSet_t ht, hash_s_t pos){
  void* elem = hashSet_getKey(ht, pos);
  return *(hash_l_t*) elem != 0; // struct has the same pointer as its first element
}

bool hashSet_tryAdd(hashSet_t ht, void *key, hash_s_t *ret_pos){
  int ret = hashSet_find(ht, key, ret_pos);

  if(ret == -1) return 0;
  if(ret == 1) return 1;
  // add

   ht->items++;
   memcpy(ht->table + *ret_pos * ht->elem_size, key, ht->elem_size);
   return 1;
}
