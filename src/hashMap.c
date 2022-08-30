#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "hashMap.h"
#include "mem.h"


typedef struct hashMap{
  void* table;
  shift_t num_elem;
  uint32_t size_k;
  uint32_t size_v;
  uint64_t hash_requests;
  uint64_t hash_lookups;
  size_t num_curr_elem;
  char *of_what;
} *hashMap_priv_t;


shift_t hashMap_getNumElem(hashMap_t ht_p){
  hashMap_priv_t ht = ht_p;
  return ht->num_elem;
}

size_t hashMap_getNumCurrElem(hashMap_t ht_p){
  hashMap_priv_t ht = ht_p;
  return ht->num_curr_elem;
}

double hashMap_dbg_fill(hashMap_t ht_p){
  hashMap_priv_t ht = ht_p;
  return ht->num_curr_elem / (double) (1ll << ht->num_elem);
}
double hashMap_dbg_hashConflictRate(hashMap_t ht_p){
  hashMap_priv_t ht = ht_p;
  return (ht->hash_lookups - ht->hash_requests) / (double) ht->hash_requests;
}

void *hashMap_new(shift_t num_elem, size_t size_k, size_t size_v, char *of_what){
  if(size_k % sizeof(uint64_t) != 0) FAIL("hashMap: %s: size_k must be a multiple of %ld", of_what, sizeof(uint64_t))
  if(size_v % sizeof(uint64_t) != 0) FAIL("hashMap: %s: size_v must be a multiple of %ld", of_what, sizeof(uint64_t))

  hashMap_priv_t ret = mem_calloc(sizeof(struct hashMap), 1, of_what);
  void *table = mem_calloc(size_k + size_v, 1ll<<num_elem, of_what);

  *ret = (struct hashMap){
    .table = table,
    .num_elem = num_elem,
    .size_k = size_k,
    .size_v = size_v,
    .hash_requests = 0,
    .hash_lookups = 0,
    .num_curr_elem = 0,
    .of_what = of_what,
  };
  return ret;
}

void hashMap_delete(hashMap_t ht_p){
  hashMap_priv_t ht = ht_p;

  mem_free(ht->table);
  mem_free(ht);
}


/*
// must be pow2
#define CACHE_LINE 64
#define NUM_ATTEMPT_HASH_S 16

static inline hash_l_t calc_hash_l(hashMap_priv_t ht, void *key_p){
  uint64_t *key = key_p;

  size_t size = ht->size_k/sizeof(uint64_t);

  hash_l_t hash = 1;
  for(size_t i = 0; i < size; i++){
    hash_l_t it = key[i];
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  }

  hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  return hash != 0 ? hash : 1;
}

static hash_s_t calc_hash_s(hashMap_priv_t ht, hash_l_t hash, hash_l_t counter){
  if(counter == 0){
    return hash & ((1ll << ht->num_elem) -1);
  }
  if(counter <= 7){
    shift_t s = counter * 8;
    return ROT(hash, s) & ((1ll << ht->num_elem) -1);
  }
  if(counter <= 15){
    shift_t s = (counter - 8) * 8;
    hash = ROT(hash, s);
    hash_s_t r = 0;
    for(int i = 0; i < 64; i+=ht->num_elem)
      r ^= (hash >> i) & ((1ll << ht->num_elem) -1);
    return r;
  }
  FAIL("hashMap: %s: bug in calc_hash_s", ht->of_what)
}

static int hashMap_find(hashMap_priv_t ht, void *key, hash_s_t *hash_elem){
  ht->hash_requests++;
  hash_l_t *base_hash = (hash_l_t*) key; // struct has the same pointer as its first element

  if(*base_hash == 0) *base_hash = calc_hash_l(ht, key);
  if(*base_hash == 0){
    printf("hashMap: %s: couldn't calculate a base_hash != 0\n", ht->of_what);
    return -1;
  }
  size_t size_tot = ht->size_k + ht->size_v;
  hash_s_t block = MIN( (hash_s_t) 1ll<<ht->num_elem, 1+ (hash_s_t) (CACHE_LINE-1)/size_tot );

  for(hash_s_t counter = 0; counter < NUM_ATTEMPT_HASH_S; counter++){
    hash_s_t hash = calc_hash_s(ht, *base_hash, counter);

    for(hash_s_t i = 0; i < block; i++){
      *hash_elem = (hash + i) & ((1ll << ht->num_elem)-1); // (hash &~(block-1)) | ((hash+i) & (block-1));
      void* elem = ht->table + *hash_elem * size_tot;
      ht->hash_lookups++;

      if(*(hash_l_t*) elem == 0) // struct has the same pointer as its first element. 0 -> uninited
        return 0;

      if(memcmp(elem, key, ht->size_k) == 0)
        return 1;
    }
  }

  printf("hashMap: %s: couldn't find a match or an empty place! fill=%d%%, conflictRate=%d%%\n", ht->of_what, (int) (hashMap_dbg_fill(ht) * 100), (int) (hashMap_dbg_hashConflictRate(ht) * 100));
  return -1;
}
*/

static inline hash_l_t calc_hash(hashMap_priv_t ht, void *key_p, hash_l_t init){
  uint64_t *key = key_p;

  size_t size = ht->size_k/sizeof(uint64_t);

  hash_l_t hash = init;
  for(size_t i = 0; i < size; i++){
    hash_l_t it = key[i];
    it = (it * 0xB0000B) ^ ROT(it, 23);
    hash ^= (it * 0xB0000B) ^ ROT(it, 23);
    hash = (hash * 0xB0000B) ^ ROT(hash, 23);
  }

  hash = (hash * 0xB0000B) ^ ROT(hash, 23);

  return hash != 0 ? hash : 1;
}

#define NUM_ATTEMPT_HASH_1 10
#define NUM_ATTEMPT_HASH_2 10

static int hashMap_find(hashMap_priv_t ht, void *key, hash_s_t *hash_elem){
  ht->hash_requests++;
  hash_l_t *base_hash = (hash_l_t*) key; // struct has the same pointer as its first element

  size_t size_tot = ht->size_k + ht->size_v;

  for(shift_t sh = 0; sh < NUM_ATTEMPT_HASH_1; sh++){
    *base_hash = calc_hash(ht, key, sh);
    hash_s_t hash = *base_hash & ((1ll << ht->num_elem) -1);

    for(hash_s_t i = 0; i < NUM_ATTEMPT_HASH_2; i++){
      *hash_elem = (hash + i) & ((1ll << ht->num_elem)-1);

      void* elem = ht->table + *hash_elem * size_tot;
      ht->hash_lookups++;

      if(*(hash_l_t*) elem == 0) // struct has the same pointer as its first element. 0 -> uninited
        return 0;

      if(memcmp(elem, key, ht->size_k) == 0)
        return 1;
    }
  }

  printf("hashMap: %s: couldn't find a match or an empty place! fill=%d%%, conflictRate=%d%%\n", ht->of_what, (int) (hashMap_dbg_fill(ht) * 100), (int) (hashMap_dbg_hashConflictRate(ht) * 100));
  return -1;
}

bool hashMap_contains(hashMap_t ht, void *key){
  hash_s_t hash_elem;
  int ret = hashMap_find(ht, key, &hash_elem);
  return ret == 1;
}

bool hashMap_add(hashMap_t ht_p, void *key, void *value){
  hashMap_priv_t ht = ht_p;

  hash_s_t hash_elem;
  int ret = hashMap_find(ht, key, &hash_elem);

  if(ret == -1) return 0;
  if(ret == 1) FAIL("hashTable: %s: key already present\n", ht->of_what)

   ht->num_curr_elem++;
   size_t size_tot = ht->size_k + ht->size_v;
   memcpy(ht->table + hash_elem * size_tot, key, ht->size_k);
   memcpy(ht->table + hash_elem * size_tot + ht->size_k, value, ht->size_v);
   return 1;
}

hash_s_t hashMap_getPos(hashMap_t ht_p, void *key){
  hashMap_priv_t ht = ht_p;

  hash_s_t hash_elem;
  int ret = hashMap_find(ht, key, &hash_elem);

  if(ret == -1) return 0;
  if(ret == 0) FAIL("hashTable: %s: missing key\n", ht->of_what)

   return hash_elem;
}

bool hashMap_validPos(hashMap_t ht, hash_s_t pos){
  void* elem = hashMap_getKey(ht, pos);
  return *(hash_l_t*) elem != 0; // struct has the same pointer as its first element
}

void* hashMap_getKey(hashMap_t ht_p, hash_s_t pos){
  hashMap_priv_t ht = ht_p;

  size_t size_tot = ht->size_k + ht->size_v;
  return ht->table + pos * size_tot;
}

void* hashMap_getValue(hashMap_t ht_p, hash_s_t pos){
  hashMap_priv_t ht = ht_p;

  size_t size_tot = ht->size_k + ht->size_v;
  return ht->table + pos * size_tot + ht->size_k;
}

bool hashMap_set(hashMap_t ht_p, void *key, void *value, hash_s_t *ret_pos){
  hashMap_priv_t ht = ht_p;
  int ret = hashMap_find(ht, key, ret_pos);

  if(ret == -1) return 0;

  size_t size_tot = ht->size_k + ht->size_v;
  memcpy(ht->table + *ret_pos * size_tot + ht->size_k, value, ht->size_v);

  if(ret == 0){
    ht->num_curr_elem++;
    memcpy(ht->table + *ret_pos * size_tot, key, ht->size_k);
  }

  return 1;
}
