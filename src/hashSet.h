#ifndef _HASH_SET_H_
#define _HASH_SET_H_

#include "types.h"
#include <stddef.h>

// NOTE: The first element of the struct must be a 'hash_l_t hash;'
// the hash_l_t hash will only depend on the value of the key.


typedef void *hashSet_t;

hashSet_t hashSet_new(shift_t num_elem, size_t size_k, char *of_what);
void hashSet_delete(hashSet_t ht);

// keys must be memset to 0

bool hashSet_contains(hashSet_t ht, void *key); // sets the hash field of key if != 0.

// adds the element and returns the permanent hash. the key must be absent. returns false on fail.
bool hashSet_add(hashSet_t ht, void *key); // sets the hash field of key if != 0.

// returns the current hash. key must have been added already
hash_s_t hashSet_getPos(hashSet_t ht, void *key); // sets the hash field of key if != 0.

// return if that pos has a valid value
bool hashSet_validPos(hashSet_t ht, hash_s_t pos);

// returns the current hash. key must have been added already
void* hashSet_getKey(hashSet_t ht, hash_s_t pos);

// like add, but succedes also if already presents. returns the pos in the param.
bool hashSet_tryAdd(hashSet_t ht, void *key, hash_s_t *ret_pos); // sets the hash field of key if != 0.

shift_t hashSet_getNumElem(hashSet_t ht);
size_t hashSet_getNumCurrElem(hashSet_t ht);



double hashSet_dbg_fill(hashSet_t ht);
double hashSet_dbg_hashConflictRate(hashSet_t ht);


#endif // _HASH_SET_H_
