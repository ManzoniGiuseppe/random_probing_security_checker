#include "hashSet.h"
#include "hashMap.h"


hashSet_t hashSet_new(shift_t num_elem, size_t size_k, char *of_what){ return hashMap_new(num_elem, size_k, 0, of_what); }
void hashSet_delete(hashSet_t ht){ return hashMap_delete(ht); }
bool hashSet_contains(hashSet_t ht, void *key){ return hashMap_contains(ht, key); }
bool hashSet_add(hashSet_t ht, void *key){ int u; return hashMap_add(ht, key, &u); }
hash_s_t hashSet_getPos(hashSet_t ht, void *key){ return hashMap_getPos(ht, key); }
bool hashSet_validPos(hashSet_t ht, hash_s_t pos){ return hashMap_validPos(ht, pos); }
void* hashSet_getKey(hashSet_t ht, hash_s_t pos){ return hashMap_getKey(ht, pos); }
bool hashSet_tryAdd(hashSet_t ht, void *key, hash_s_t *ret_pos){ int u; return hashMap_set(ht, key, &u, ret_pos); }
shift_t hashSet_getNumElem(hashSet_t ht){ return hashMap_getNumElem(ht); }
size_t hashSet_getNumCurrElem(hashSet_t ht){ return hashMap_getNumCurrElem(ht); }
double hashSet_dbg_fill(hashSet_t ht){ return hashMap_dbg_fill(ht); }
double hashSet_dbg_hashConflictRate(hashSet_t ht){ return hashMap_dbg_hashConflictRate(ht); }
