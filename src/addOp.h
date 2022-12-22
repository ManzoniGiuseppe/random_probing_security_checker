//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _ADDOP_H_
#define _ADDOP_H_

#include <stdint.h>
#include "types.h"
#include "hash.h"



typedef struct { void *addOp; } addOp_t; // the storage of the add

typedef uint64_t addOp_arr_t;   // an array of values
typedef struct {
  void *info;
  T__THREAD_SAFE addOp_arr_t (*fast)(void *info, addOp_arr_t val);  // can be NULL
  T__THREAD_SAFE void (*leaf)(void *info, hash_t val, bool isPos, hash_t *ret_val, bool *ret_isPos); // can be NULL if the other is != NULL
} addOp_1Info_t;   // it describes an operation

typedef struct {
  void *info;
  int same_op1;
  int opposite_op1;

  T__THREAD_SAFE bool (*isFastLeaf)(void *info, int paramNum, hash_t val, bool isNeg);
  T__THREAD_SAFE addOp_arr_t (*fastLeaf)(void *info, int paramNum, hash_t val, bool isPos, addOp_arr_t other);

  T__THREAD_SAFE void (*leaf)(void *info, hash_t val[2], bool isPos[2], hash_t *ret_val, bool *ret_isPos);
} addOp_2Info_t;   // it describes an operation




T__THREAD_SAFE addOp_t addOp_new(int numOp1, int numOp2, T__ACQUIRES addOp_1Info_t *op1s, T__ACQUIRES addOp_2Info_t *op2s);
void addOp_delete(addOp_t storage);


T__THREAD_SAFE addOp_arr_t addOp_leaf(hash_t val, bool isPos);
addOp_arr_t addOp_node(addOp_t storage, wire_t inputBit, addOp_arr_t val0, addOp_arr_t val1); // val0 and val1 must have a higher inputBit, or be a const.
T__THREAD_SAFE addOp_arr_t addOp_neg(addOp_arr_t p);

addOp_arr_t addOp_op1(addOp_t storage, int op, addOp_arr_t p);
addOp_arr_t addOp_op2(addOp_t storage, int op, addOp_arr_t p0, addOp_arr_t p1);

T__THREAD_SAFE void addOp_flattenR(addOp_t s, addOp_arr_t p, wire_t maxDepth, hash_t *ret_hash, bool *ret_isPos);

T__THREAD_SAFE double addOp_dbg_storageFill(addOp_t storage);
T__THREAD_SAFE double addOp_dbg_hashConflictRate(addOp_t storage);


// TODO: remove
T__THREAD_SAFE bool addOp_isLeafElseNode(addOp_arr_t p);
T__THREAD_SAFE void addOp_getLeaf(addOp_arr_t p, hash_t *ret_val, bool *ret_isPos); // if a ret_ is null, it's not set
T__THREAD_SAFE void addOp_getNode(addOp_t storage, addOp_arr_t p, wire_t *ret_inputBit, addOp_arr_t *ret_val0, addOp_arr_t *ret_val1); // if a ret_ is null, it's not set



#endif // _ADD_H_
