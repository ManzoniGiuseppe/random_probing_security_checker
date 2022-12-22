//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "bdd.h"
#include "mem.h"
#include "addOp.h"
#include "hashCache.h"
#include "string.h"


#define OP1_ID  0
#define OP1_0   1
#define OP1_1   2
#define OP2_AND  0
#define OP2_XOR  1


#define BDD_CONST(x)  addOp_leaf((hash_t){0}, (x))

T__THREAD_SAFE static addOp_arr_t op1_id(__attribute__((unused)) void *info, addOp_arr_t val){ return val; }
T__THREAD_SAFE static addOp_arr_t op1_0(__attribute__((unused)) void *info, __attribute__((unused)) addOp_arr_t val){ return BDD_CONST(0); }
T__THREAD_SAFE static addOp_arr_t op1_1(__attribute__((unused)) void *info, __attribute__((unused)) addOp_arr_t val){ return BDD_CONST(1); }
T__THREAD_SAFE bool always_isFastLeaf(__attribute__((unused)) void *info, __attribute__((unused)) int paramNum, __attribute__((unused)) hash_t val, __attribute__((unused)) bool isPos){ return 1; }
T__THREAD_SAFE void op2_and_leaf(__attribute__((unused)) void *info, __attribute__((unused)) hash_t val[2], bool isPos[2], hash_t *ret_val, bool *ret_isPos){
  *ret_val = (hash_t){0};
  *ret_isPos = isPos[0] && isPos[1];
}
T__THREAD_SAFE addOp_arr_t op2_and_fastLeaf(__attribute__((unused)) void *info, __attribute__((unused)) int paramNum, __attribute__((unused)) hash_t val, bool isPos, addOp_arr_t other){
  if(isPos) return other;
  else return BDD_CONST(0);
}
T__THREAD_SAFE void op2_xor_leaf(__attribute__((unused)) void *info, __attribute__((unused)) hash_t val[2], bool isPos[2], hash_t *ret_val, bool *ret_isPos){
  *ret_val = (hash_t){0};
  *ret_isPos = isPos[0] ^ isPos[1];
}
T__THREAD_SAFE addOp_arr_t op2_xor_fastLeaf(__attribute__((unused)) void *info, __attribute__((unused)) int paramNum, __attribute__((unused)) hash_t val, bool isPos, addOp_arr_t other){
  if(isPos) return addOp_neg(other);
  else return other;
}


typedef struct {
  addOp_t a;
  addOp_1Info_t *op1s;
  addOp_2Info_t *op2s;
} bdd_storage_t;
#define P(pub)   ((bdd_storage_t *) ((bdd_t) (pub)).bdd)


#define B2A(x)  (P(x)->a)

bdd_t bdd_storage_alloc(void){
  bdd_t ret = { mem_calloc(sizeof(bdd_storage_t), 1, "BDD main struct") };

  P(ret)->op1s = mem_calloc(sizeof(addOp_1Info_t), 3, "BDD array of op1");
  P(ret)->op1s[OP1_ID].info = NULL;
  P(ret)->op1s[OP1_ID].fast = op1_id;
  P(ret)->op1s[OP1_ID].leaf = NULL;

  P(ret)->op1s[OP1_0].info = NULL;
  P(ret)->op1s[OP1_0].fast = op1_0;
  P(ret)->op1s[OP1_0].leaf = NULL;

  P(ret)->op1s[OP1_1].info = NULL;
  P(ret)->op1s[OP1_1].fast = op1_1;
  P(ret)->op1s[OP1_1].leaf = NULL;

  P(ret)->op2s = mem_calloc(sizeof(addOp_2Info_t), 2, "BDD array of op1");
  P(ret)->op2s[OP2_AND].info = NULL;
  P(ret)->op2s[OP2_AND].same_op1 = OP1_ID;
  P(ret)->op2s[OP2_AND].opposite_op1 = OP1_0;
  P(ret)->op2s[OP2_AND].isFastLeaf = always_isFastLeaf;
  P(ret)->op2s[OP2_AND].fastLeaf = op2_and_fastLeaf;
  P(ret)->op2s[OP2_AND].leaf = op2_and_leaf;

  P(ret)->op2s[OP2_XOR].info = NULL;
  P(ret)->op2s[OP2_XOR].same_op1 = OP1_0;
  P(ret)->op2s[OP2_XOR].opposite_op1 = OP1_1;
  P(ret)->op2s[OP2_XOR].isFastLeaf = always_isFastLeaf;
  P(ret)->op2s[OP2_XOR].fastLeaf = op2_xor_fastLeaf;
  P(ret)->op2s[OP2_XOR].leaf = op2_xor_leaf;

  P(ret)->a = addOp_new(3, 2, P(ret)->op1s, P(ret)->op2s);
  return ret;
}
void bdd_storage_free(bdd_t s){
  addOp_delete(B2A(s));
  mem_free(P(s)->op1s);
  mem_free(P(s)->op2s);
  mem_free(P(s));
}

bdd_fn_t bdd_val_const(__attribute__((unused)) bdd_t s, bool val){ return BDD_CONST(val); }
bdd_fn_t bdd_val_single(bdd_t s, wire_t inputBit){ return addOp_node(B2A(s), inputBit, BDD_CONST(0), BDD_CONST(1)); }

bdd_fn_t bdd_op_not(__attribute__((unused)) bdd_t s, bdd_fn_t val){ return addOp_neg(val); }
bdd_fn_t bdd_op_and(bdd_t s, bdd_fn_t val0, bdd_fn_t val1){ return addOp_op2(B2A(s), OP2_AND, val0, val1); }
bdd_fn_t bdd_op_or(bdd_t s, bdd_fn_t val0, bdd_fn_t val1){ return addOp_neg(bdd_op_and(s, addOp_neg(val0), addOp_neg(val1))); }
bdd_fn_t bdd_op_xor(bdd_t s, bdd_fn_t val0, bdd_fn_t val1){ return addOp_op2(B2A(s), OP2_XOR, val0, val1); }

T__THREAD_SAFE void bdd_flattenR(bdd_t s, bdd_fn_t p, wire_t maxDepth, hash_t *ret_hash, bool *ret_isPos){ addOp_flattenR(B2A(s), p, maxDepth, ret_hash, ret_isPos); }

T__THREAD_SAFE double bdd_dbg_storageFill(bdd_t s){ return addOp_dbg_storageFill(B2A(s)); }
T__THREAD_SAFE double bdd_dbg_hashConflictRate(bdd_t s){ return addOp_dbg_hashConflictRate(B2A(s)); }







// -- cacheSum



typedef struct {
  hashCache_t cacheSum;
  bdd_t storage;
  size_t numberSize;
  wire_t numTotIns;
  wire_t numMaskedIns;
  wire_t numRnds;
} bdd_sumCached_storage_t;
#define PC(pub)   ((bdd_sumCached_storage_t *) ((bdd_sumCached_t) (pub)).bdd_sumCached)


typedef struct{ bdd_fn_t param; } cacheSum_elem_t;

#define CACHE_SUM_ADD(cached, V, RET, CODE) {\
  cacheSum_elem_t CACHE_ADD__key;\
  memset(&CACHE_ADD__key, 0, sizeof(cacheSum_elem_t));\
  CACHE_ADD__key.param = (V);\
  if(!hashCache_getValue(PC(cached)->cacheSum, &CACHE_ADD__key, &RET)){\
    { CODE }\
    hashCache_set(PC(cached)->cacheSum, &CACHE_ADD__key, &RET);\
  }\
}



T__THREAD_SAFE bdd_sumCached_t bdd_sumCached_new(bdd_t storage, wire_t numTotIns, wire_t numMaskedIns){
  bdd_sumCached_t ret = { mem_calloc(sizeof(bdd_sumCached_storage_t), 1, "the sumCache's storage for BDDs!") };
  PC(ret)->cacheSum = hashCache_new(sizeof(cacheSum_elem_t), sizeof(number_t) * NUMBER_SIZE(numTotIns + 3), "bdd's cacheSum");
  PC(ret)->storage = storage;
  PC(ret)->numTotIns = numTotIns;
  PC(ret)->numMaskedIns = numMaskedIns;
  PC(ret)->numberSize = NUMBER_SIZE(numTotIns + 3);
  PC(ret)->numRnds = numTotIns - numMaskedIns;
  return ret;
}
void bdd_sumCached_delete(bdd_sumCached_t cached){
  hashCache_delete(PC(cached)->cacheSum);
  mem_free(PC(cached));
}



// -- sumRnd



T__THREAD_SAFE static void sumRnd(bdd_sumCached_t c, bdd_fn_t val, number_t *ret){
  size_t size = PC(c)->numberSize;

  if(addOp_isLeafElseNode(val)){
    number_one(size, PC(c)->numRnds, ret);
    bool isPos;
    addOp_getLeaf(val, NULL, &isPos);

    if(isPos)   // (-1)^F = (-1)^0 = 1        (-1)^T = (-1)^1 = -1
      number_local_negation(size, ret);
    return;
  }

  CACHE_SUM_ADD(c, val, *ret, {
    bdd_fn_t v[2];
    addOp_getNode(B2A(PC(c)->storage), val, NULL, &v[0], &v[1]);

    number_t r0[size];
    sumRnd(c, v[0], r0);

    number_t r1[size];
    sumRnd(c, v[1], r1);

    number_add(size, r0, r1, ret);
    number_local_div2(size, ret);
  })
}




// -- transform




T__THREAD_SAFE static void doSubTransform_block(bdd_sumCached_t c, number_t *ret, size_t fixed, size_t input, shift_t blockSize){ // a block of 2^{blockSize} leaves all the same
  if(blockSize == 0) return; //this if is for optimization. nothing changes if removed.
  size_t numSize = PC(c)->numberSize;

  // the first element of the transform is the sum of them, which means the value times the number of values
  number_local_lshift(numSize, & ret[fixed * numSize], blockSize);
  // the other elements of the transform contain a difference and are all zero
  for(size_t i = 1; i < 1ull << blockSize; i++)
    number_zero(numSize, & ret[((i << input) | fixed) * numSize]);
}


T__THREAD_SAFE static void doSubTransform(bdd_sumCached_t c, bdd_fn_t val, number_t *ret, size_t lowest, size_t input){
  size_t numMaskedIns = PC(c)->numMaskedIns;
  size_t numSize = PC(c)->numberSize;

  if(addOp_isLeafElseNode(val)){
    sumRnd(c, val, & ret[lowest * numSize]);
    doSubTransform_block(c, ret, lowest, input, numMaskedIns - input);
    return;
  }

  wire_t in;
  bdd_fn_t a[2];
  addOp_getNode(B2A(PC(c)->storage), val, &in, &a[0], &a[1]);

  if(in >= numMaskedIns){
    sumRnd(c, val, & ret[lowest * numSize]);
    doSubTransform_block(c, ret, lowest, input, numMaskedIns - input);
    return;
  }

  doSubTransform(c, a[0], ret, lowest, in+1);
  doSubTransform(c, a[1], ret, (1ll << in) | lowest, in+1);

  // calculate this step of the fast walsh-hadamard transform

  //    u   m lowest
  //   /  \/ \/\   .
  // 89786543210
  //  |   |  |
  // nmi in  input
  for(size_t u = 0; u < 1ull << (numMaskedIns - (in+1)); u++){
    number_t *v0 = & ret[((u << (in+1)) | lowest) * numSize];
    number_t *v1 = & ret[((u << (in+1)) | (1ll << in) | lowest) * numSize];

    number_t sum[numSize];
    number_t diff[numSize];

    number_add(numSize, v0, v1, sum);
    number_sub(numSize, v0, v1, diff);

    number_copy(numSize, sum, v0);
    number_copy(numSize, diff, v1);
  }

  // if a set of  m   bits are all the same, call the calculation for a block
  if(in != input){ // this if is only for optimization, there is one already inside the cycle.
    for(size_t u = 0; u < 1ull << (numMaskedIns - in); u++){
      doSubTransform_block(c, ret, (u << in) | lowest, input, in - input);
    }
  }
}



T__THREAD_SAFE void bdd_sumCached_transform(bdd_sumCached_t cached, bdd_fn_t val, number_t *ret){  // ret contains  1ll << numMaskedIns  blocks of  PC(pub)->numBits  num_t
  doSubTransform(cached, val, ret, 0, 0);
}
