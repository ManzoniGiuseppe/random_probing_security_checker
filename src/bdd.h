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


//#define MAX_NUM_TOT_INS


#define BDD_FLAGS 1

typedef uint64_t bdd_fn_t;
#define MAX_NUM_TOT_INS__LOG2   (64 - HASH_WIDTH - BDD_FLAGS)

// HASH_WIDTH = 48  means MAX_NUM_TOT_INS__LOG2 = 64 -48 - BDD_FLAGS = 15, which means it can handle MAX_NUM_TOT_INS = 32768 wires.
// lowering HASH_WIDTH increases the MAX_NUM_TOT_INS__LOG2, which allows to support more wires.
#if HASH_WIDTH >= 48
  #error "unsupported 48 used of indexes, a computer with that much memory is outside the current target."
#endif
#if MAX_NUM_TOT_INS >= 32768
  #error "unsupported MAX_NUM_TOT_INS >= 32768"
#endif


typedef struct { void *bdd; } bdd_t;



bdd_t bdd_storage_alloc(void);
void bdd_storage_free(bdd_t storage);

bdd_fn_t bdd_val_const(bdd_t storage, bool val);
bdd_fn_t bdd_val_single(bdd_t storage, wire_t inputBit);

bdd_fn_t bdd_op_not(bdd_t storage, bdd_fn_t val);
bdd_fn_t bdd_op_and(bdd_t storage, bdd_fn_t val0, bdd_fn_t val1);
bdd_fn_t bdd_op_xor(bdd_t storage, bdd_fn_t val0, bdd_fn_t val1);

T__THREAD_SAFE void bdd_get_flattenedInputs(bdd_t storage, bdd_fn_t val, wire_t numMaskedIns, bdd_fn_t *ret); // ret[1ll << numMaskedIns];
T__THREAD_SAFE fixed_cell_t bdd_get_sumRandomsPN1(bdd_t storage, bdd_fn_t val, wire_t numRnds);

T__THREAD_SAFE double bdd_dbg_storageFill(bdd_t storage);
T__THREAD_SAFE double bdd_dbg_hashConflictRate(bdd_t storage);


#endif // _BDD_H_
