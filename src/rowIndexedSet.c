//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "rowIndexedSet.h"
#include "mem.h"
#include "bitArray.h"
#include "hashSet.h"


#define DBG_FILE "rowIndexedSet"
#define DBG_LVL DBG_ROWINDEXEDSET



typedef struct{
  hash_compact_t *hashed2valIndex;
  hashSet_t vals;
  pow2size_t numRowHashed;
  char *ofWhat;
} rowIndexedSet_storage_t;
#define P(pub)   ((rowIndexedSet_storage_t *) ((rowIndexedSet_t)(pub)).rowIndexedSet)


T__THREAD_SAFE rowIndexedSet_t rowIndexedSet_new(pow2size_t numRowHashed, size_t size_v, T__ACQUIRES char *ofWhat){
  DBG(DBG_LVL_MINIMAL, "%s: new, with numRowHashed=0x%lX, size_v=%ld\n", ofWhat, (long) numRowHashed, (long) size_v)
  rowIndexedSet_t ret = { mem_calloc(sizeof(rowIndexedSet_storage_t), 1, ofWhat) };

  P(ret)->hashed2valIndex = mem_calloc(sizeof(hash_compact_t), numRowHashed, ofWhat);
  P(ret)->numRowHashed = numRowHashed;
  P(ret)->ofWhat = ofWhat;
  P(ret)->vals = hashSet_new(size_v, ofWhat);
  return ret;
}

void rowIndexedSet_delete(rowIndexedSet_t s){
  DBG(DBG_LVL_MINIMAL, "%s: delete\n", P(s)->ofWhat)
  mem_free(P(s)->hashed2valIndex);
  hashSet_delete(P(s)->vals);
  mem_free(P(s));
}

// key may or may not be present. return if afterward they are inserted.
void rowIndexedSet_add(rowIndexedSet_t s, rowHash_t row, void *val){
  DBG(DBG_LVL_DETAILED, "%s: add at row=0x%lX\n", P(s)->ofWhat, (long) row.v)
  P(s)->hashed2valIndex[row.v] = hash_toCompact(hashSet_add(P(s)->vals, val));
}

T__THREAD_SAFE pow2size_t rowIndexedSet_currSize(rowIndexedSet_t s){
  DBG(DBG_LVL_MAX, "of %s\n", P(s)->ofWhat)

  pow2size_t ret = hashSet_getNumElem(P(s)->vals);
  DBG(DBG_LVL_DETAILED, "%s: get currSize: %ld bits\n", P(s)->ofWhat, (long) ret)
  return ret;
}

T__THREAD_SAFE bool rowIndexedSet_containsRow(rowIndexedSet_t s, rowHash_t row){
  bool ret = P(s)->hashed2valIndex[row.v].v != 0;
  DBG(DBG_LVL_DETAILED, "%s: containsRow, row=0x%lX is%s contained\n", P(s)->ofWhat, (long) row.v, ret?"":" not")
  return ret;
}

T__THREAD_SAFE bool rowIndexedSet_hasVal(rowIndexedSet_t s, reducedHash_t valIndex, void **ret_key){
  return hashSet_hasVal(P(s)->vals, valIndex, ret_key);
}

T__THREAD_SAFE hash_t rowIndexedSet_row2valIndex(rowIndexedSet_t s, rowHash_t row){
  DBG(DBG_LVL_MAX, "row2valIndex, entered with param rowHashed=0x%lX\n", (long) row.v)
  DBG(DBG_LVL_MAX, "row2valIndex, of %s\n", P(s)->ofWhat)

  ON_DBG(DBG_LVL_TOFIX, {
    if(!rowIndexedSet_containsRow(s, row)) FAIL("rowIndexedSet_row2valIndex: %s: Trying to get the missing row 0x%llX\n", P(s)->ofWhat, (long long) row.v)
  })
  hash_t ret = hash_fromCompact(P(s)->hashed2valIndex[row.v]);
  DBG(DBG_LVL_DETAILED, "%s: row2valIndex, row=0x%lX, valIndex=%ld\n", P(s)->ofWhat, (long) row.v, (long) ret.v)
  return ret;
}

T__THREAD_SAFE T__LEAKS void* rowIndexedSet_getVal(rowIndexedSet_t s, hash_t valIndex){
  DBG(DBG_LVL_DETAILED, "%s: getVal, valIndex=0x%lX\n", P(s)->ofWhat, (long) valIndex.v)
  return hashSet_getKey(P(s)->vals, valIndex);
}
