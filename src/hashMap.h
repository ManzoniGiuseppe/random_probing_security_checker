#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include "types.h"
#include <stddef.h>

// NOTE: The first element of the struct must be a 'hash_l_t hash;'
// the hash_l_t hash will only depend on the value of the key.

typedef void *hashMap_t;

hashMap_t hashMap_new(shift_t num_elem, size_t size_k, size_t size_v, char *of_what);
void hashMap_delete(hashMap_t ht);

// keys must be memset to 0

bool hashMap_contains(hashMap_t ht, void *key); // sets the hash field of key if != 0.

// adds the element and returns the permanent hash. the key must be absent. returns false on fail.
bool hashMap_add(hashMap_t ht, void* key, void* value); // sets the hash field of key if != 0.

// returns the current hash. key must have been added already
hash_s_t hashMap_getPos(hashMap_t ht, void* key); // sets the hash field of key if != 0.

// return if that pos has a valid value
bool hashMap_validPos(hashMap_t ht, hash_s_t pos);

// returns the current hash. key must have been added already
void* hashMap_getKey(hashMap_t ht, hash_s_t pos);

// returns the current hash. key must have been added already
// this is mutable
void* hashMap_getValue(hashMap_t ht, hash_s_t pos);

// like add, but succedes also if already presents. returns the pos in the param.
bool hashMap_set(hashMap_t ht, void *key, void *value, hash_s_t *ret_pos); // sets the hash field of key if != 0.

shift_t hashMap_getNumElem(hashMap_t ht);
size_t hashMap_getNumCurrElem(hashMap_t ht);


double hashMap_dbg_fill(hashMap_t ht);
double hashMap_dbg_hashConflictRate(hashMap_t ht);


#endif // _HASH_MAP_H_
