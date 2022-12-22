//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <string.h>

#include "addCore.h"
#include "mem.h"
#include "hashSet.h"
#include "hashMap.h"
#include "hashCache.h"



//#define NUM_THREADS




#define DBG_FILE  "addCore"
#define DBG_LVL  DBG_ADDCORE



// -- storageless



//   63: input | flags | hash :0
#define ADD_FLAG__NEG (((addCore_arr_t) 1) << HASH_WIDTH)
#define add_getHash(v) ((hash_t){ (v) & MASK_OF(HASH_WIDTH) })
#define CONST_MAGIC_INPUT  MASK_OF(MAX_NUM_TOT_INS__LOG2)

#define add_isPos(x)  (((x) & ADD_FLAG__NEG) == 0)
#define add_getInput(x) ((x) >> (HASH_WIDTH + ADD_FLAGS))
#define add_toInput(in)  (((addCore_arr_t) (in)) << (HASH_WIDTH + ADD_FLAGS))


T__THREAD_SAFE addCore_arr_t addCore_neg(addCore_arr_t p){
  return p ^ ADD_FLAG__NEG;
}

T__THREAD_SAFE bool addCore_isLeafElseNode(addCore_arr_t p){
  return add_getInput(p) == CONST_MAGIC_INPUT;
}

T__THREAD_SAFE addCore_arr_t addCore_leaf(hash_t val, bool isPos){
//  addCore_arr_t ret = ((addCore_arr_t)val) | add_toInput(CONST_MAGIC_INPUT) | (isNegated ? ADD_FLAG__NEG : 0);
  addCore_arr_t ret = val.v;
  ret |= add_toInput(CONST_MAGIC_INPUT);
  ret |= isPos ? 0 : ADD_FLAG__NEG;
  return ret;
}

T__THREAD_SAFE void addCore_getLeaf(addCore_arr_t p, hash_t *val, bool *isPos){
  if(val) *val = add_getHash(p);
  if(isPos) *isPos = add_isPos(p);
}




// -- storage & dbg



typedef struct{ addCore_arr_t v[2]; } node_t;
#define A2S(pub)   ((hashSet_t) { ((addCore_t) (pub)).addCore })

T__THREAD_SAFE addCore_t addCore_new(void){
  hashSet_t ret = hashSet_new(sizeof(node_t), "ADD, the storage");
  return (addCore_t){ ret.hashSet };
}

void addCore_delete(addCore_t s){ hashSet_delete(A2S(s)); }
double addCore_dbg_storageFill(addCore_t s){ return hashSet_dbg_fill(A2S(s)); }
double addCore_dbg_hashConflictRate(addCore_t s){ return hashSet_dbg_hashConflictRate(A2S(s)); }



// -- getNode


T__THREAD_SAFE void addCore_getNode(addCore_t s, addCore_arr_t p, wire_t *inputBit, addCore_arr_t *val0, addCore_arr_t *val1){
  node_t ret = *(node_t*)hashSet_getKey(A2S(s), add_getHash(p));
  if(!add_isPos(p)){
    ret.v[0] = addCore_neg(ret.v[0]);
    ret.v[1] = addCore_neg(ret.v[1]);
  }
  if(val0) *val0 = ret.v[0];
  if(val1) *val1 = ret.v[1];
  if(inputBit) *inputBit = add_getInput(p);
}


// -- add_node


addCore_arr_t addCore_node(addCore_t s, wire_t inputBit, addCore_arr_t sub0, addCore_arr_t sub1){
  if(sub0 == sub1) return sub0; // simplify
  if(!add_isPos(sub0)) return addCore_neg(addCore_node(s, inputBit, addCore_neg(sub0), addCore_neg(sub1))); // ensure it's unique: have the sub0 always positive

  hash_t hash;
  {
    node_t key;
    memset(&key, 0, sizeof(node_t));
    key.v[0] = sub0;
    key.v[1] = sub1;
    hash = hashSet_add(A2S(s), &key);
  }
  return ((addCore_arr_t)hash.v) | add_toInput(inputBit);
}


// -- add_flatten

T__THREAD_SAFE static void flattenR(addCore_t s, addCore_arr_t p, wire_t maxDepth, wire_t currDepth, hash_t *ret_hash, bool *ret_isPos){
  if(currDepth > maxDepth) FAIL("addCore_flatten: currDepth > maxDepth\n");

  size_t blockSize = 1ull<< (maxDepth-currDepth);

  if(addCore_isLeafElseNode(p)){
    for(size_t i = 0; i < blockSize; i++){
      if(ret_hash) ret_hash[i] = add_getHash(p);
      if(ret_isPos) ret_isPos[i] = add_isPos(p);
    }
  }else{
    size_t h = blockSize>>1;
    hash_t *next_hash = ret_hash ? ret_hash + h : NULL;
    bool *next_isPos = ret_isPos ? ret_isPos + h : NULL;

    if(add_getInput(p) == currDepth){
      addCore_arr_t sub[2];
      addCore_getNode(s, p, NULL, &sub[0], &sub[1]);

      flattenR(s, sub[0], maxDepth, currDepth+1, ret_hash, ret_isPos);
      flattenR(s, sub[1], maxDepth, currDepth+1, next_hash, next_isPos);
    }else{
      flattenR(s, p, maxDepth, currDepth+1, ret_hash, ret_isPos);
      flattenR(s, p, maxDepth, currDepth+1, next_hash, next_isPos);
    }
  }
}

T__THREAD_SAFE void addCore_flattenR(addCore_t s, addCore_arr_t p, wire_t maxDepth, hash_t *ret_hash, bool *ret_isPos){
  flattenR(s, p, maxDepth, 0, ret_hash, ret_isPos);
}
