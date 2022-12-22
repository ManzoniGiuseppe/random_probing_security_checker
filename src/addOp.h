//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _ADDOP_H_
#define _ADDOP_H_

#include <stdint.h>
#include <string.h>
#include "addCore.h"
#include "hashCache.h"
#include "types.h"
#include "hash.h"



#define DBG_FILE  "addOp"
#define DBG_LVL  DBG_ADDOP


typedef struct{ addCore_arr_t param[2]; } addOp_cacheKey_t;
typedef addCore_arr_t addOp_cacheVal_t;


#define ADDOP__CACHE_ADD(cacheOp, p0, p1, RET, CODE) {\
  addOp_cacheKey_t CACHE_ADD__key;\
  memset(&CACHE_ADD__key, 0, sizeof(addOp_cacheKey_t));\
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
        addOp_cacheVal_t CACHE_ADD__r;\
        if(!hashCache_getValue(cacheOp, &CACHE_ADD__key, &CACHE_ADD__r)) FAIL("add: cacheOp's hashCache couldn't find a valud just added.")\
        if(CACHE_ADD__r != RET) FAIL("add: cacheOp's hashCache returned a value that wasn't what inserted.")\
      }\
    })\
  }\
}



inline addCore_arr_t addOp_unary(
  addCore_t s,
  addCore_arr_t p,
  void *info,
  T__THREAD_SAFE void (*leaf)(void *info, hash_t val, bool isPos, hash_t *ret_val, bool *ret_isPos),
  hashCache_t cache,
  addCore_arr_t (*recursion)(void *info, addCore_t s, addCore_arr_t p)
){
  if(addCore_isLeafElseNode(p)){
    hash_t val;
    bool isPos;
    addCore_getLeaf(p, &val, &isPos);

    hash_t ret_val;
    bool ret_isPos;
    leaf(info, val, isPos, &ret_val, &ret_isPos);

    return addCore_leaf(ret_val, ret_isPos);
  }

  addCore_arr_t ret;
  ADDOP__CACHE_ADD(cache, p, p, ret, {
    wire_t inputBit;
    addCore_arr_t val0;
    addCore_arr_t val1;
    addCore_getNode(s, p, &inputBit, &val0, &val1);

    val0 = recursion(info, s, val0);
    val1 = recursion(info, s, val1);

    ret = addCore_node(s, inputBit, val0, val1);
  })
  return ret;
}


inline addCore_arr_t addOp_binary(
  addCore_t s,
  addCore_arr_t p0,
  addCore_arr_t p1,
  void *info,
  T__THREAD_SAFE addCore_arr_t (*same_op)(void *info, addCore_arr_t val),
  T__THREAD_SAFE addCore_arr_t (*opposite_op)(void *info, addCore_arr_t val),
  T__THREAD_SAFE bool (*isFastLeaf)(void *info, int paramNum, hash_t val, bool isNeg),
  T__THREAD_SAFE addCore_arr_t (*fastLeaf)(void *info, int paramNum, hash_t val, bool isPos, addCore_arr_t other),
  T__THREAD_SAFE void (*fullLeaf)(void *info, hash_t val[2], bool isPos[2], hash_t *ret_val, bool *ret_isPos),
  hashCache_t cache,
  addCore_arr_t (*recursion)(void *info, addCore_t s, addCore_arr_t p0, addCore_arr_t p1)
){
  if(p0 == p1) return same_op(info, p0);
  if(p0 == addCore_neg(p1)) return opposite_op(info, p0);

  addCore_arr_t p[2] = {p0, p1};

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
    fullLeaf(info, val, isPos, &ret_val, &ret_isPos);

    return addCore_leaf(ret_val, ret_isPos);
  }

  for(int k = 0; k < 2; k++)
    if(c[k]){
      hash_t val;
      bool isPos;
      addCore_getLeaf(p[k], &val, &isPos);

      if(isFastLeaf(info, k, val, isPos))
        return fastLeaf(info, k, val, isPos, p[1-k]);
    }


  addCore_arr_t ret;
  ADDOP__CACHE_ADD(cache, p0, p1, ret, {
    addCore_arr_t a[2][2];
    wire_t w[2];

    for(int k = 0; k < 2; k++)
      addCore_getNode(s, p[k], &w[k], &a[k][0], &a[k][1]);

    int min = w[0] <= w[1] ? 0 : 1;  // parameter with the minimal input, minimal -> first.
    int max = 1-min; // parameter with the other input.

    addCore_arr_t sub[2];
    if(w[min] == w[max]){
      for(int l = 0; l < 2; l++)
        sub[l] = recursion(info, s, a[0][l], a[1][l]);
    }else if(min == 0){
      for(int l = 0; l < 2; l++)
        sub[l] = recursion(info, s, a[0][l], p[1]);   // only open the one with the first input, leave the other unchanged.
    }else{
      for(int l = 0; l < 2; l++)
        sub[l] = recursion(info, s, p[0], a[1][l]);   // only open the one with the first input, leave the other unchanged.
    }

    ret = addCore_node(s, w[min], sub[0], sub[1]);
  })
  return ret;
}


#undef DBG_FILE
#undef DBG_LVL



#endif // _ADD_H_
