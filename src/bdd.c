//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bdd.h"
#include "mem.h"
#include "hashSet.h"
#include "hashMap.h"
#include "hashCache.h"



#define DBG_FILE  "bdd"
#define DBG_LVL  DBG_BDD



// -- storageless


//   63 input | flags | hash 0

#define BDD_FLAG__NOT (((bdd_fn_t) 1) << HASH_WIDTH)
#define BDD_HASH(v) ((hash_t){ (v) & MASK_OF(HASH_WIDTH) })

#define bdd_op_neg(x)  ((x) ^ BDD_FLAG__NOT)
#define bdd_is_negated(x)  (((x) & BDD_FLAG__NOT) != 0)

#define bdd_get_input(x) ((x) >> (HASH_WIDTH + BDD_FLAGS))
#define bdd_place_input(in)  (((bdd_fn_t) (in)) << (HASH_WIDTH + BDD_FLAGS))


#define LEAF_MAGIC_INPUT  MASK_OF(MAX_NUM_TOT_INS__LOG2)
#define bdd_is_leaf(x)  (bdd_get_input(x) == LEAF_MAGIC_INPUT)


#define BDD_TRUE   bdd_place_input(LEAF_MAGIC_INPUT)
#define BDD_FALSE  bdd_op_neg(BDD_TRUE)



bdd_fn_t bdd_op_not(__attribute__((unused)) bdd_t storage, bdd_fn_t val){
  return bdd_op_neg(val);
}
bdd_fn_t bdd_val_const(__attribute__((unused)) bdd_t storage, bool val){
  return val ? BDD_TRUE : BDD_FALSE;
}



// -- storage

typedef struct{ bdd_fn_t v[2]; } bdd_node_t;

typedef struct{ bdd_fn_t param[2]; } cacheOp_elem_t;
typedef struct{ bdd_fn_t param; wire_t numRnds; } cacheSum_elem_t;

typedef struct {
  hashSet_t storage;
  hashCache_t cacheAnd;
  hashCache_t cacheXor;
  hashCache_t cacheSum;
} bdd_storage_t;
#define P(pub)   ((bdd_storage_t *) ((bdd_t) (pub)).bdd)

bdd_t bdd_storage_alloc(void){
  bdd_t ret = { mem_calloc(sizeof(bdd_storage_t), 1, "the storage for BDDs!") };
  P(ret)->storage = hashSet_new(sizeof(bdd_node_t), "bdd's storage");
  P(ret)->cacheAnd = hashCache_new(sizeof(cacheOp_elem_t), sizeof(bdd_fn_t), "bdd's cacheAnd");
  P(ret)->cacheXor = hashCache_new(sizeof(cacheOp_elem_t), sizeof(bdd_fn_t), "bdd's cacheXor");
  P(ret)->cacheSum = hashCache_new(sizeof(cacheSum_elem_t), sizeof(fixed_cell_t), "bdd's cacheSum");
  return ret;
}

void bdd_storage_free(bdd_t s){
  hashSet_delete(P(s)->storage);
  hashCache_delete(P(s)->cacheAnd);
  hashCache_delete(P(s)->cacheXor);
  hashCache_delete(P(s)->cacheSum);
  mem_free(P(s));
}


// -- get

T__THREAD_SAFE static inline bdd_node_t bdd_get_node(bdd_t s, bdd_fn_t x){
  bdd_node_t ret = *(bdd_node_t*)hashSet_getKey(P(s)->storage, BDD_HASH(x));
  if(bdd_is_negated(x)){
    ret.v[0] = bdd_op_neg(ret.v[0]);
    ret.v[1] = bdd_op_neg(ret.v[1]);
  }
  return ret;
}

double bdd_dbg_storageFill(bdd_t s){
  return hashSet_dbg_fill(P(s)->storage);
}

double bdd_dbg_hashConflictRate(bdd_t s){
  return hashSet_dbg_hashConflictRate(P(s)->storage);
}


// -- bdd_dbg_print


static void bdd_dbg_print__i(int dbg_lvl, bdd_t s, bdd_fn_t val, int padding){
  if(bdd_is_leaf(val)){
    if(val == BDD_TRUE){
      char toPrint[padding + strlen("TRUE\n") + 1];
      for(int i = 0; i < padding; i++) toPrint[i] = ' ';
      strcpy(toPrint+padding, "TRUE\n");
      DBG(dbg_lvl, "%s", toPrint)
    }else{
      char toPrint[padding + strlen("FALSE\n") + 1];
      for(int i = 0; i < padding; i++) toPrint[i] = ' ';
      strcpy(toPrint+padding, "FALSE\n");
      DBG(dbg_lvl, "%s", toPrint)
    }
    return;
  }

  {
    char toPrint[padding + 4 + 10 + 1];
    for(int i = 0; i < padding; i++) toPrint[i] = ' ';
    sprintf(toPrint + padding, "{ %lld:\n", (long long) bdd_get_input(val));
    DBG(dbg_lvl, "%s", toPrint)
  }

  bdd_node_t a = bdd_get_node(s, val);
  bdd_dbg_print__i(dbg_lvl, s, a.v[0], padding+2);
  bdd_dbg_print__i(dbg_lvl, s, a.v[1], padding+2);

  {
    char toPrint[padding + 4 + 10 + 1];
    for(int i = 0; i < padding; i++) toPrint[i] = ' ';
    strcpy(toPrint+padding, "}\n");
    DBG(dbg_lvl, "%s", toPrint)
  }
}

static inline void bdd_dbg_print(int dbg_lvl, bdd_t s, bdd_fn_t val){
  ON_DBG(dbg_lvl, {
    bdd_dbg_print__i(dbg_lvl, s, val, 0);
  })
}


// -- bdd_new_node


static bdd_fn_t bdd_new_node(bdd_t s, bdd_fn_t sub0, bdd_fn_t sub1, wire_t input){
  if(sub0 == sub1) return sub0; // simplify
  if(bdd_is_negated(sub0)) return bdd_op_neg(bdd_new_node(s, bdd_op_neg(sub0), bdd_op_neg(sub1), input)); // ensure it's unique: have the sub0 always positive

  hash_t hash;
  {
    bdd_node_t key;
    memset(&key, 0, sizeof(bdd_node_t));
    key.v[0] = sub0;
    key.v[1] = sub1;
    hash = hashSet_add(P(s)->storage, &key);
  }
  bdd_fn_t ret = ((bdd_fn_t)hash.v) | bdd_place_input(input);

  DBG(DBG_LVL_DETAILED, "New node, input=%ld, sub={%llX, %llX} ret=%llX\n", (long) input, (long long) sub0, (long long) sub1, (long long) ret);

  ON_DBG(DBG_LVL_TOFIX, {
    bdd_node_t a = bdd_get_node(s, ret);
    if(a.v[0] != sub0) FAIL("bdd_new_node, mismatch inserted sub0=%llX, a.v[0]=%llX\n", sub0, a.v[0]);
    if(a.v[1] != sub1) FAIL("bdd_new_node, mismatch inserted sub1=%llX, a.v[1]=%llX\n", sub1, a.v[1]);
  })

  bdd_dbg_print(DBG_LVL_MAX, s, ret);

  return ret;
}


// -- bdd_val_single


bdd_fn_t bdd_val_single(bdd_t s, wire_t inputBit){
  return bdd_new_node(s, BDD_FALSE, BDD_TRUE, inputBit);
}


// -- cache


//#define CACHE_OP_ADD(cacheOp, op, p0, p1, RET, CODE) CODE


#define CACHE_OP_ADD(cacheOp, p0, p1, RET, CODE) {\
  cacheOp_elem_t CACHE_ADD__key;\
  memset(&CACHE_ADD__key, 0, sizeof(cacheOp_elem_t));\
  CACHE_ADD__key.param[0] = (p0);\
  CACHE_ADD__key.param[1] = (p1);\
\
  if(hashCache_getValue(cacheOp, &CACHE_ADD__key, &RET)){\
    ON_DBG(DBG_LVL_TOFIX, {\
      if(RET == 0) FAIL("bdd: cacheOp's hashCache just returned a 0, which is an illegal index.\n")\
    })\
  }else{\
    { CODE }\
    hashCache_set(cacheOp, &CACHE_ADD__key, &RET);\
    ON_DBG(DBG_LVL_TOFIX, {\
      if(NUM_THREADS == 1){\
        bdd_fn_t CACHE_ADD__r;\
        if(!hashCache_getValue(cacheOp, &CACHE_ADD__key, &CACHE_ADD__r)) FAIL("bdd: cacheOp's hashCache couldn't find a valud just added.")\
        if(CACHE_ADD__r != RET) FAIL("bdd: cacheOp's hashCache returned a value that wasn't what inserted.")\
      }\
    })\
  }\
}

#define CACHE_SUM_ADD(cacheSum, rnds, V, RET, CODE) {\
  cacheSum_elem_t CACHE_ADD__key;\
  memset(&CACHE_ADD__key, 0, sizeof(cacheSum_elem_t));\
  CACHE_ADD__key.param = (V);\
  CACHE_ADD__key.numRnds = (rnds);\
  if(!hashCache_getValue(cacheSum, &CACHE_ADD__key, &RET)){\
    { CODE }\
    hashCache_set(cacheSum, &CACHE_ADD__key, &RET);\
    ON_DBG(DBG_LVL_TOFIX, {\
      if(NUM_THREADS == 1){\
        fixed_cell_t CACHE_ADD__r;\
        if(!hashCache_getValue(cacheSum, &CACHE_ADD__key, &CACHE_ADD__r)) FAIL("bdd: cacheOp's hashCache couldn't find a valud just added.")\
        if(CACHE_ADD__r != RET) FAIL("bdd: cacheOp's hashCache returned a value that wasn't what inserted.")\
      }\
    })\
  }\
}


// -- bdd_op_and


bdd_fn_t bdd_op_and(bdd_t s, bdd_fn_t val0, bdd_fn_t val1){
  DBG(DBG_LVL_DETAILED, "bdd_op_and, v0=%llX  v1=%llX\n", (long long) val0, (long long) val1);
  bdd_dbg_print(DBG_LVL_MAX, s, val0);
  bdd_dbg_print(DBG_LVL_MAX, s, val1);

  bdd_fn_t val[2] = { val0, val1 };
  for(int i = 0; i < 2; i++){
    if(bdd_is_leaf(val[i])){
      if(val[i] == BDD_FALSE) return BDD_FALSE;
      else return val[1-i]; // return the other
    }
  }

  bdd_fn_t ret;
  CACHE_OP_ADD(P(s)->cacheAnd, val0, val1, ret, {
    bdd_node_t a[2];
    a[0] = bdd_get_node(s, val0);
    a[1] = bdd_get_node(s, val1);
    wire_t i[2];
    i[0] = bdd_get_input(val0);
    i[1] = bdd_get_input(val1);

    int min = i[0] <= i[1] ? 0 : 1;  // parameter with the minimal input, minimal -> first.
    int max = 1-min; // parameter with the other input.

    bdd_fn_t sub[2];
    if(i[min] == i[max]){
      sub[0] = bdd_op_and(s, a[0].v[0], a[1].v[0]); // sub 0, v[0]
      sub[1] = bdd_op_and(s, a[0].v[1], a[1].v[1]);
    }else{
      sub[0] = bdd_op_and(s, a[min].v[0], val[max]);  // sub 0, v[0]
      sub[1] = bdd_op_and(s, a[min].v[1], val[max]); // only open the one with the first input, leave the other unchanged.
    }
    ret = bdd_new_node(s, sub[0], sub[1], i[min]);
  })
  return ret;
}


// -- bdd_op_xor


bdd_fn_t bdd_op_xor(bdd_t s, bdd_fn_t val0, bdd_fn_t val1){
  DBG(DBG_LVL_DETAILED, "bdd_op_xor, v0=%llX  v1=%llX\n", (long long) val0, (long long) val1);
  bdd_dbg_print(DBG_LVL_MAX, s, val0);
  bdd_dbg_print(DBG_LVL_MAX, s, val1);

  bdd_fn_t val[2] = { val0, val1 };
  for(int i = 0; i < 2; i++){
    if(bdd_is_leaf(val[i])){
      DBG(DBG_LVL_DETAILED, "param %d is the leaf %s\n", i, val[i] == BDD_FALSE ? "F" : "T");
      if(val[i] == BDD_FALSE) return val[1-i]; // return the other
      else return bdd_op_neg(val[1-i]); // xor true = not
    }
  }

  bdd_fn_t ret;
  CACHE_OP_ADD(P(s)->cacheXor, val0, val1, ret, {
    bdd_node_t a[2];
    wire_t i[2];
    for(int j = 0; j < 2; j++){
      a[j] = bdd_get_node(s, val[j]);
      i[j] = bdd_get_input(val[j]);
    }

    int min = i[0] < i[1] ? 0 : 1;  // parameter with the minimal input, minimal -> first.
    int max = 1-min; // parameter with the other input.

    bdd_fn_t sub[2];
    if(i[min] == i[max]){
      sub[0] = bdd_op_xor(s, a[0].v[0], a[1].v[0]); // sub 0, v[0]
      sub[1] = bdd_op_xor(s, a[0].v[1], a[1].v[1]);

      DBG(DBG_LVL_MAX, "a[0], i[0]=%d\n", i[0]);
      bdd_dbg_print(DBG_LVL_MAX, s, a[0].v[0]);
      bdd_dbg_print(DBG_LVL_MAX, s, a[0].v[1]);
      DBG(DBG_LVL_MAX, "a[1], i[1]=%d\n", i[1]);
      bdd_dbg_print(DBG_LVL_MAX, s, a[1].v[0]);
      bdd_dbg_print(DBG_LVL_MAX, s, a[1].v[1]);
      DBG(DBG_LVL_MAX, "sub, i=%d\n", i[min]);
      bdd_dbg_print(DBG_LVL_MAX, s, sub[0]);
      bdd_dbg_print(DBG_LVL_MAX, s, sub[1]);
    }else{
      sub[0] = bdd_op_xor(s, a[min].v[0], val[max]);  // sub 0, v[0]
      sub[1] = bdd_op_xor(s, a[min].v[1], val[max]); // only open the one with the first input, leave the other unchanged.

      DBG(DBG_LVL_MAX, "a[min], i[min]=%d\n", i[min]);
      bdd_dbg_print(DBG_LVL_MAX, s, a[min].v[0]);
      bdd_dbg_print(DBG_LVL_MAX, s, a[min].v[1]);
      DBG(DBG_LVL_MAX, "a[max]\n");
      bdd_dbg_print(DBG_LVL_MAX, s, val[max]);
      DBG(DBG_LVL_MAX, "sub, i=%d\n", i[min]);
      bdd_dbg_print(DBG_LVL_MAX, s, sub[0]);
      bdd_dbg_print(DBG_LVL_MAX, s, sub[1]);
    }
    ret = bdd_new_node(s, sub[0], sub[1], i[min]);
  })
  return ret;
}


// -- getFlatened



T__THREAD_SAFE static void getFlatened(bdd_t s, bdd_fn_t val, bdd_fn_t *ret, wire_t numMaskedIns, size_t lowest, shift_t input){
  if(bdd_is_leaf(val)){
    DBG(DBG_LVL_MAX, "getFlatened of leaf %s with numMaskedIns=%ld, ret=%llX, lowest=%ld, input=%ld\n", val == BDD_TRUE ? "T" : "F", (long) numMaskedIns, ret, (long) lowest, (long) input);
    for(size_t i = 0; i < 1ull << (numMaskedIns - input); i++)
      ret[(i << input) | lowest] = val;
    DBG(DBG_LVL_MAX, "leaf's return set\n");
    return;
  }
  DBG(DBG_LVL_MAX, "getFlatened of %llX with numMaskedIns=%ld, ret=%llX, lowest=%ld, input=%ld\n", (long long) val, (long) numMaskedIns, ret, (long) lowest, (long) input);

  bdd_node_t a = bdd_get_node(s, val);
  wire_t in = bdd_get_input(val);

  if(in >= numMaskedIns){
    for(size_t i = 0; i < 1ull << (numMaskedIns - input); i++)
      ret[(i << input) | lowest] = val;
    DBG(DBG_LVL_MAX, "return set, all bdd nodes relative to randoms\n");
    return;
  }

  DBG(DBG_LVL_MAX, "getFlatened of %llX, in=%ld\n", (long long) val, (long) in);
  getFlatened(s, a.v[0], ret, numMaskedIns, lowest, in+1);
  getFlatened(s, a.v[1], ret, numMaskedIns, lowest | (1ll << in), in+1);

  //    u   m lowest
  //   /  \/ \/\   .
  // 89786543210
  //  |   |  |
  // nmi in  input
  for(size_t u = 0; u < 1ull << (numMaskedIns - in); u++)
    for(size_t m = 1; m < 1ull << (in - input); m++)
      ret[(u << in) | (m << input) | lowest] = ret[(u << in) | lowest];
  DBG(DBG_LVL_MAX, "getFlatened of $llX's return set\n", (long long) val);
}

T__THREAD_SAFE void bdd_get_flattenedInputs(bdd_t s, bdd_fn_t val, wire_t numMaskedIns, bdd_fn_t *ret){
  DBG(DBG_LVL_DETAILED, "bdd_get_flattenedInputs of %llX with numMaskedIns=%d\n", (long long) val, numMaskedIns);
  getFlatened(s, val, ret, numMaskedIns, 0, 0);
}



T__THREAD_SAFE static fixed_cell_t sumRandomsPN1(bdd_t s, bdd_fn_t val, wire_t numRnds, int depth){
  DBG(DBG_LVL_MAX, "sumRandomsPN1 with val=%llX numRnds=%ld depth=%d\n", (long long) val, (long) numRnds, depth);
  if(bdd_is_negated(val)) return -sumRandomsPN1(s, bdd_op_neg(val), numRnds, depth+1);
  if(bdd_is_leaf(val)) return -( ((fixed_cell_t) 1) << numRnds );

  fixed_cell_t ret;
  CACHE_SUM_ADD(P(s)->cacheSum, numRnds, val, ret, {
    bdd_node_t a = bdd_get_node(s, val);
    bdd_fn_t v0 = a.v[0];
    bdd_fn_t v1 = a.v[1];
    DBG(DBG_LVL_MAX, "sumRandomsPN1: start sub nodes %llX and %llX\n", (long long) v0, (long long) v1);
    fixed_cell_t r0 = sumRandomsPN1(s, v0, numRnds, depth+1);
    DBG(DBG_LVL_MAX, "sumRandomsPN1: val = %llX, r0 = %f\n", (long long) val, (double) r0);
    fixed_cell_t r1 = sumRandomsPN1(s, v1, numRnds, depth+1);
    DBG(DBG_LVL_MAX, "sumRandomsPN1: val = %llX, r1 = %f\n", (long long) val, (double) r1);
    ret = (r0 + r1)/2; // /2 as each covers half the column space
  })
  return ret;
}

T__THREAD_SAFE fixed_cell_t bdd_get_sumRandomsPN1(bdd_t s, bdd_fn_t val, wire_t numRnds){
  DBG(DBG_LVL_DETAILED, "bdd_get_sumRandomsPN1 of %llX with numRnds=%d\n", (long long) val, numRnds);
  fixed_cell_t ret = sumRandomsPN1(s, val, numRnds, 1);
  DBG(DBG_LVL_DETAILED, "bdd_get_sumRandomsPN1 of %llX is %f\n", (long long) val, (double) ret);
  return ret;
}
