//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _ADDCORE_H_
#define _ADDCORE_H_

#include <stdint.h>
#include "types.h"
#include "hash.h"


//#define MAX_NUM_TOT_INS


#define ADD_FLAGS 1

#define MAX_NUM_TOT_INS__LOG2   (64 - HASH_WIDTH - ADD_FLAGS)

// HASH_WIDTH = 48  means log2(MAX_NUM_TOT_INS+1) = 64 -48 - ADD_FLAGS = 15, which means it can handle MAX_NUM_TOT_INS = 32767 wires.
// lowering HASH_WIDTH increases the log2(MAX_NUM_TOT_INS+1), which allows to support more wires.
#if HASH_WIDTH >= 48
  #error "unsupported 48 used of indexes, a computer with that much memory is outside the current target."
#endif
#if MAX_NUM_TOT_INS >= 32767
  #error "unsupported MAX_NUM_TOT_INS >= 32767"
#endif


typedef struct { void *addCore; } addCore_t; // the storage of the add
typedef uint64_t addCore_arr_t;   // an array of values


T__THREAD_SAFE addCore_t addCore_new(void);
void addCore_delete(addCore_t storage);


T__THREAD_SAFE addCore_arr_t addCore_leaf(hash_t val, bool isPos);
addCore_arr_t addCore_node(addCore_t storage, wire_t inputBit, addCore_arr_t val0, addCore_arr_t val1); // val0 and val1 must have a higher inputBit, or be a const.

T__THREAD_SAFE bool addCore_isLeafElseNode(addCore_arr_t p);
T__THREAD_SAFE void addCore_getLeaf(addCore_arr_t p, hash_t *ret_val, bool *ret_isPos); // if a ret_ is null, it's not set
T__THREAD_SAFE void addCore_getNode(addCore_t storage, addCore_arr_t p, wire_t *ret_inputBit, addCore_arr_t *ret_val0, addCore_arr_t *ret_val1); // if a ret_ is null, it's not set

T__THREAD_SAFE addCore_arr_t addCore_neg(addCore_arr_t p);

T__THREAD_SAFE void addCore_flattenR(addCore_t s, addCore_arr_t p, wire_t maxDepth, hash_t *ret_hash, bool *ret_isPos);

T__THREAD_SAFE double addCore_dbg_storageFill(addCore_t storage);
T__THREAD_SAFE double addCore_dbg_hashConflictRate(addCore_t storage);



#endif // _ADDCORE_H_
