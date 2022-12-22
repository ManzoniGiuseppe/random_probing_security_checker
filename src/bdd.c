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



#define BDD_CONST(x)  addCore_leaf((hash_t){0}, (x))

T__THREAD_SAFE static inline addCore_arr_t op1_id(__attribute__((unused)) void *info, addCore_arr_t val){ return val; }
T__THREAD_SAFE static inline addCore_arr_t op1_0(__attribute__((unused)) void *info, __attribute__((unused)) addCore_arr_t val){ return BDD_CONST(0); }
T__THREAD_SAFE static inline addCore_arr_t op1_1(__attribute__((unused)) void *info, __attribute__((unused)) addCore_arr_t val){ return BDD_CONST(1); }

T__THREAD_SAFE static inline bool always_isFastLeaf(__attribute__((unused)) void *info, __attribute__((unused)) int paramNum, __attribute__((unused)) hash_t val, __attribute__((unused)) bool isPos){ return 1; }
T__THREAD_SAFE static inline void op2_and_fullLeaf(__attribute__((unused)) void *info, __attribute__((unused)) hash_t val[2], bool isPos[2], hash_t *ret_val, bool *ret_isPos){
  *ret_val = (hash_t){0};
  *ret_isPos = isPos[0] && isPos[1];
}
T__THREAD_SAFE static inline addCore_arr_t op2_and_fastLeaf(__attribute__((unused)) void *info, __attribute__((unused)) int paramNum, __attribute__((unused)) hash_t val, bool isPos, addCore_arr_t other){
  if(isPos) return other;
  else return BDD_CONST(0);
}
T__THREAD_SAFE static inline void op2_xor_fullLeaf(__attribute__((unused)) void *info, __attribute__((unused)) hash_t val[2], bool isPos[2], hash_t *ret_val, bool *ret_isPos){
  *ret_val = (hash_t){0};
  *ret_isPos = isPos[0] ^ isPos[1];
}
T__THREAD_SAFE static inline addCore_arr_t op2_xor_fastLeaf(__attribute__((unused)) void *info, __attribute__((unused)) int paramNum, __attribute__((unused)) hash_t val, bool isPos, addCore_arr_t other){
  if(isPos) return addCore_neg(other);
  else return other;
}

static addCore_arr_t fn_and(void *info, addCore_t s, addCore_arr_t p0, addCore_arr_t p1){
  hashCache_t *cache = info;
  return addOp_binary(s, p0, p1, info, op1_id, op1_0, always_isFastLeaf, op2_and_fastLeaf, op2_and_fullLeaf, *cache, fn_and);
}

static addCore_arr_t fn_xor(void *info, addCore_t s, addCore_arr_t p0, addCore_arr_t p1){
  hashCache_t *cache = info;
  return addOp_binary(s, p0, p1, info, op1_0, op1_1, always_isFastLeaf, op2_xor_fastLeaf, op2_xor_fullLeaf, *cache, fn_xor);
}



typedef struct {
  addCore_t a;
  hashCache_t cache_and;
  hashCache_t cache_xor;
} bdd_storage_t;
#define P(pub)   ((bdd_storage_t *) ((bdd_t) (pub)).bdd)


#define B2A(x)  (P(x)->a)

bdd_t bdd_storage_alloc(void){
  bdd_t ret = { mem_calloc(sizeof(bdd_storage_t), 1, "BDD main struct") };

  P(ret)->a = addCore_new();
  P(ret)->cache_and = hashCache_new(sizeof(addOp_cacheKey_t), sizeof(addOp_cacheVal_t), "BDD, a cache for and");
  P(ret)->cache_xor = hashCache_new(sizeof(addOp_cacheKey_t), sizeof(addOp_cacheVal_t), "BDD, a cache for xor");

  return ret;
}
void bdd_storage_free(bdd_t s){
  addCore_delete(B2A(s));
  hashCache_delete(P(s)->cache_and);
  hashCache_delete(P(s)->cache_xor);
  mem_free(P(s));
}

bdd_fn_t bdd_val_const(__attribute__((unused)) bdd_t s, bool val){ return BDD_CONST(val); }
bdd_fn_t bdd_val_single(bdd_t s, wire_t inputBit){ return addCore_node(B2A(s), inputBit, BDD_CONST(0), BDD_CONST(1)); }

bdd_fn_t bdd_op_not(__attribute__((unused)) bdd_t s, bdd_fn_t val){ return addCore_neg(val); }
bdd_fn_t bdd_op_and(bdd_t s, bdd_fn_t val0, bdd_fn_t val1){ return fn_and(&P(s)->cache_and, B2A(s), val0, val1); }
bdd_fn_t bdd_op_xor(bdd_t s, bdd_fn_t val0, bdd_fn_t val1){ return fn_xor(&P(s)->cache_xor, B2A(s), val0, val1); }
bdd_fn_t bdd_op_or(bdd_t s, bdd_fn_t val0, bdd_fn_t val1){ return addCore_neg(bdd_op_and(s, addCore_neg(val0), addCore_neg(val1))); }

T__THREAD_SAFE void bdd_flattenR(bdd_t s, bdd_fn_t p, wire_t maxDepth, hash_t *ret_hash, bool *ret_isPos){ addCore_flattenR(B2A(s), p, maxDepth, ret_hash, ret_isPos); }

T__THREAD_SAFE double bdd_dbg_storageFill(bdd_t s){ return addCore_dbg_storageFill(B2A(s)); }
T__THREAD_SAFE double bdd_dbg_hashConflictRate(bdd_t s){ return addCore_dbg_hashConflictRate(B2A(s)); }







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

  if(addCore_isLeafElseNode(val)){
    number_one(size, PC(c)->numRnds, ret);
    bool isPos;
    addCore_getLeaf(val, NULL, &isPos);

    if(isPos)   // (-1)^F = (-1)^0 = 1        (-1)^T = (-1)^1 = -1
      number_local_negation(size, ret);
    return;
  }

  CACHE_SUM_ADD(c, val, *ret, {
    bdd_fn_t v[2];
    addCore_getNode(B2A(PC(c)->storage), val, NULL, &v[0], &v[1]);

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

  if(addCore_isLeafElseNode(val)){
    sumRnd(c, val, & ret[lowest * numSize]);
    doSubTransform_block(c, ret, lowest, input, numMaskedIns - input);
    return;
  }

  wire_t in;
  bdd_fn_t a[2];
  addCore_getNode(B2A(PC(c)->storage), val, &in, &a[0], &a[1]);

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
