//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _BDD_H_
#define _BDD_H_

#include <stdint.h>
#include "types.h"
#include "hash.h"
#include "number.h"




typedef uint64_t bdd_fn_t;
typedef struct { void *bdd; } bdd_t;


bdd_t bdd_storage_alloc(void);
void bdd_storage_free(bdd_t storage);

bdd_fn_t bdd_val_const(bdd_t storage, bool val);
bdd_fn_t bdd_val_single(bdd_t storage, wire_t inputBit);

bdd_fn_t bdd_op_not(bdd_t storage, bdd_fn_t val);
bdd_fn_t bdd_op_and(bdd_t storage, bdd_fn_t val0, bdd_fn_t val1);
bdd_fn_t bdd_op_or(bdd_t storage, bdd_fn_t val0, bdd_fn_t val1);
bdd_fn_t bdd_op_xor(bdd_t storage, bdd_fn_t val0, bdd_fn_t val1);

T__THREAD_SAFE void bdd_flattenR(bdd_t s, bdd_fn_t p, wire_t maxDepth, hash_t *ret_hash, bool *ret_isPos);

T__THREAD_SAFE double bdd_dbg_storageFill(bdd_t storage);
T__THREAD_SAFE double bdd_dbg_hashConflictRate(bdd_t storage);





typedef struct { void *bdd_sumCached; } bdd_sumCached_t;

T__THREAD_SAFE bdd_sumCached_t bdd_sumCached_new(bdd_t storage, wire_t numTotIns, wire_t numMaskedIns);
void bdd_sumCached_delete(bdd_sumCached_t cached);


T__THREAD_SAFE void bdd_sumCached_transform(bdd_sumCached_t cached, bdd_fn_t val, number_t *ret);  // ret contains  1ll << numMaskedIns  blocks of  NUM_SIZE(numTotIns+3)  num_t







#endif // _BDD_H_
