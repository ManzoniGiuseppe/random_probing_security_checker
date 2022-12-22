//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "addOp.h"
#include "addCore.h"
#include "mem.h"
#include "hashCache.h"



//#define NUM_THREADS




#define DBG_FILE  "addOp"
#define DBG_LVL  DBG_ADDOP



// -- storage



typedef struct{ addOp_arr_t param[2]; } cacheKey_t;
typedef addOp_arr_t cacheVal_t;


typedef struct {
  addCore_t storage;
  int numOp1;
  int numOp2;
  addOp_1Info_t *op1s;
  addOp_2Info_t *op2s;
  hashCache_t *cacheOp1;
  hashCache_t *cacheOp2;
} add_storage_t;
#define P(pub)   ((add_storage_t *) ((addOp_t) (pub)).addOp)
#define O2C(pub)  (P(pub)->storage)

T__THREAD_SAFE addOp_t addOp_new(int numOp1, int numOp2, T__ACQUIRES addOp_1Info_t *op1s, T__ACQUIRES addOp_2Info_t *op2s){
  addOp_t ret = { mem_calloc(sizeof(add_storage_t), 1, "ADD, the main struct") };
  P(ret)->storage = addCore_new();
  P(ret)->numOp1 = numOp1;
  P(ret)->numOp2 = numOp2;
  P(ret)->op1s = op1s;
  P(ret)->op2s = op2s;
  P(ret)->cacheOp1 = mem_calloc(sizeof(hashCache_t), numOp1, "ADD, the arry of caches for op1");
  P(ret)->cacheOp2 = mem_calloc(sizeof(hashCache_t), numOp2, "ADD, the arry of caches for op2");
  for(int i = 0; i < numOp1; i++)
    P(ret)->cacheOp1[i] = hashCache_new(sizeof(cacheKey_t), sizeof(cacheVal_t), "ADD, a cache for op1");
  for(int i = 0; i < numOp2; i++)
    P(ret)->cacheOp2[i] = hashCache_new(sizeof(cacheKey_t), sizeof(cacheVal_t), "ADD, a cache for op2");
  return ret;
}

void addOp_delete(addOp_t s){
  addCore_delete(P(s)->storage);
  for(int i = 0; i < P(s)->numOp1; i++)
    hashCache_delete(P(s)->cacheOp1[i]);
  for(int i = 0; i < P(s)->numOp2; i++)
    hashCache_delete(P(s)->cacheOp2[i]);
  mem_free(P(s)->cacheOp1);
  mem_free(P(s)->cacheOp2);
  mem_free(P(s));
}



// -- forward


T__THREAD_SAFE addOp_arr_t addOp_neg(addOp_arr_t p){ return addCore_neg(p); }
T__THREAD_SAFE addOp_arr_t addOp_leaf(hash_t val, bool isPos){ return addCore_leaf(val, isPos); }
addOp_arr_t addOp_node(addOp_t s, wire_t inputBit, addOp_arr_t val0, addOp_arr_t val1){ return addCore_node(O2C(s), inputBit, val0, val1); }

T__THREAD_SAFE bool addOp_isLeafElseNode(addOp_arr_t p){ return addCore_isLeafElseNode(p); }
T__THREAD_SAFE void addOp_getLeaf(addOp_arr_t p, hash_t *ret_val, bool *ret_isPos){ addCore_getLeaf(p, ret_val, ret_isPos); }
T__THREAD_SAFE void addOp_getNode(addOp_t s, addOp_arr_t p, wire_t *ret_inputBit, addOp_arr_t *ret_val0, addOp_arr_t *ret_val1){ addCore_getNode(O2C(s), p, ret_inputBit, ret_val0, ret_val1); }

T__THREAD_SAFE void addOp_flattenR(addOp_t s, addOp_arr_t p, wire_t maxDepth, hash_t *ret_hash, bool *ret_isPos){ addCore_flattenR(O2C(s), p, maxDepth, ret_hash, ret_isPos); }

double addOp_dbg_storageFill(addOp_t s){ return addCore_dbg_storageFill(O2C(s)); }
double addOp_dbg_hashConflictRate(addOp_t s){ return addCore_dbg_hashConflictRate(O2C(s)); }




// -- cache



#define CACHE_ADD(cacheOp, p0, p1, RET, CODE) {\
  cacheKey_t CACHE_ADD__key;\
  memset(&CACHE_ADD__key, 0, sizeof(cacheKey_t));\
  CACHE_ADD__key.param[0] = (p0);\
  CACHE_ADD__key.param[1] = (p1);\
\
  if(hashCache_getValue(cacheOp, &CACHE_ADD__key, &RET)){\
    ON_DBG(DBG_LVL_TOFIX, {\
      if(RET == 0) FAIL("add: cacheOp's hashCache just returned a 0, which is an illegal index.\n")\
    })\
  }else{\
    { CODE }\
    hashCache_set(cacheOp, &CACHE_ADD__key, &RET);\
    ON_DBG(DBG_LVL_TOFIX, {\
      if(NUM_THREADS == 0){\
        cacheVal_t CACHE_ADD__r;\
        if(!hashCache_getValue(cacheOp, &CACHE_ADD__key, &CACHE_ADD__r)) FAIL("add: cacheOp's hashCache couldn't find a valud just added.")\
        if(CACHE_ADD__r != RET) FAIL("add: cacheOp's hashCache returned a value that wasn't what inserted.")\
      }\
    })\
  }\
}



// -- op1



addOp_arr_t addOp_op1(addOp_t s, int op, addOp_arr_t p){
  addOp_1Info_t *i = & P(s)->op1s[op];

  if(i->fast) return i->fast(i->info, p);
  if(addCore_isLeafElseNode(p)){
    hash_t val;
    bool isPos;
    addCore_getLeaf(p, &val, &isPos);

    hash_t ret_val;
    bool ret_isPos;
    i->leaf(i->info, val, isPos, &ret_val, &ret_isPos);

    return addOp_leaf(ret_val, ret_isPos);
  }

  addOp_arr_t ret;
  CACHE_ADD(P(s)->cacheOp1[op], p, p, ret, {
    wire_t inputBit;
    addOp_arr_t val0;
    addOp_arr_t val1;
    addCore_getNode(O2C(s), p, &inputBit, &val0, &val1);

    val0 = addOp_op1(s, op, val0);
    val1 = addOp_op1(s, op, val1);

    ret = addOp_node(s, inputBit, val0, val1);
  })
  return ret;
}



// -- op2



addOp_arr_t addOp_op2(addOp_t s, int op, addOp_arr_t p0, addOp_arr_t p1){
  addOp_2Info_t *i = & P(s)->op2s[op];
  if(p0 == p1) return addOp_op1(s, i->same_op1, p0);
  if(p0 == addOp_neg(p1)) return addOp_op1(s, i->opposite_op1, p0);

  addOp_arr_t p[2] = {p0, p1};

  bool c[2];
  for(int k = 0; k < 2; k++)
    c[k] = addCore_isLeafElseNode(p[k]);

  if(c[0] && c[1]){
    hash_t val[2];
    bool isPos[2];
    for(int k = 0; k < 2; k++)
      addCore_getLeaf(p[k], &val[k], &isPos[k]);

    hash_t ret_val;
    bool ret_isPos;
    i->leaf(i->info, val, isPos, &ret_val, &ret_isPos);

    return addOp_leaf(ret_val, ret_isPos);
  }

  for(int k = 0; k < 2; k++)
    if(c[k]){
      hash_t val;
      bool isPos;
      addCore_getLeaf(p[k], &val, &isPos);

      if(i->isFastLeaf(i->info, k, val, isPos))
        return i->fastLeaf(i->info, k, val, isPos, p[1-k]);
    }


  addOp_arr_t ret;
  CACHE_ADD(P(s)->cacheOp2[op], p0, p1, ret, {
    addOp_arr_t a[2][2];
    wire_t w[2];

    for(int k = 0; k < 2; k++)
      addCore_getNode(O2C(s), p[k], &w[k], &a[k][0], &a[k][1]);

    int min = w[0] <= w[1] ? 0 : 1;  // parameter with the minimal input, minimal -> first.
    int max = 1-min; // parameter with the other input.

    addOp_arr_t sub[2];
    if(w[min] == w[max]){
      for(int l = 0; l < 2; l++)
        sub[l] = addOp_op2(s, op, a[0][l], a[1][l]);
    }else if(min == 0){
      for(int l = 0; l < 2; l++)
        sub[l] = addOp_op2(s, op, a[0][l], p[1]);   // only open the one with the first input, leave the other unchanged.
    }else{
      for(int l = 0; l < 2; l++)
        sub[l] = addOp_op2(s, op, p[0], a[1][l]);   // only open the one with the first input, leave the other unchanged.
    }

    ret = addOp_node(s, w[min], sub[0], sub[1]);
  })
  return ret;
}

