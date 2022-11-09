//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "subrowHashed.h"
#include "rowIndexedSet.h"
#include "mem.h"



#define DBG_FILE "subrowHashed"
#define DBG_LVL DBG_SUBROWHASHED


typedef struct{
  subrowHash_t *row2subrow;
  rowHash_t *subrow2row; // one row of the many
  pow2size_t rowsSize;
  pow2size_t subrowsSize;
} subrowHashed_storage_t;
#define P(pub)   ((subrowHashed_storage_t *) ((subrowHashed_t)(pub)).subrowHashed)


T__THREAD_SAFE subrowHashed_t subrowHashed_new(rowHashed_t rows, bitArray_iterator_t itRows, void *infoR2I, hash_t (*row2info)(void* infoR2I, rowHash_t row)){
  DBG(DBG_LVL_MINIMAL, "new\n")
  pow2size_t rowsSize = rowHashed_getSize(rows);
  wire_t numTotOuts = rowHashed_numTotOuts(rows);

  DBG(DBG_LVL_DETAILED, "creating support rowIndexedSet to get which are duplicate...\n")
  rowIndexedSet_t s = rowIndexedSet_new(rowsSize, sizeof(hash_compact_t) * (numTotOuts+1), "subrowHashed's unique");

  DBG(DBG_LVL_DETAILED, "filling it up...\n")
  BITARRAY_ITERATE(numTotOuts, itRows, it, {
    rowHash_t row = rowHashed_hash(rows, it);
    DBG(DBG_LVL_MAX, "generating val for rowIndexedSet, for row with hash %lld\n", (long long) row.v)
    hash_compact_t val[numTotOuts+1];
    {
      memset(val, 0, sizeof(hash_compact_t) * (numTotOuts+1));
      val[numTotOuts] = hash_toCompact(row2info(infoR2I, row)); // the current row info
      DBG(DBG_LVL_MAX, "rowHash 0x%llX corresponds to infoHash 0x%llX\n", (long long) row.v, (long long) val[numTotOuts].v)
      int k = 0;
      ITERATE_OVER_DIRECT_SUB_ROWS(numTotOuts, it, sub, { // use the direct subrows, as they recursively include the info of all subrows.
        if(rowHashed_contains(rows, sub)){
          val[k] = hash_toCompact(rowIndexedSet_row2valIndex(s, rowHashed_hash(rows, sub)));
        } // missing are already 0, which is always an invalid hash, so no conflict.
        k++;
      });
    }
    DBG(DBG_LVL_MAX, "adding val in rowIndexedSet, for row with hash %lld\n", (long long) row.v)
    rowIndexedSet_add(s, row, val);
  })

  DBG(DBG_LVL_DETAILED, "creating memory.\n")
  subrowHashed_t ret = { mem_calloc(sizeof(subrowHashed_storage_t), 1, "subrowHashed's main structure") };

  pow2size_t subrowsSize = rowIndexedSet_currSize(s);
  P(ret)->row2subrow = mem_calloc(sizeof(subrowHash_t), rowsSize, "subrowHashed's row2subrow");
  P(ret)->subrow2row = mem_calloc(sizeof(rowHash_t), subrowsSize, "subrowHashed's subrow2row");
  // 0 is invalid
  P(ret)->rowsSize = rowsSize;
  P(ret)->subrowsSize = subrowsSize;

  DBG(DBG_LVL_DETAILED, "filling up the two way maps.\n")
  ROWHASHED_ITERATE(rows, rowIndex, {
    ON_DBG(DBG_LVL_TOFIX, {
      if(rowIndex.v == 0) FAIL("subrowHashed's new; filling up the two way maps, the rowIndex=0\n")
    })
    hash_t k = rowIndexedSet_row2valIndex(s, rowIndex);
    subrowHash_t subrowIndex = (subrowHash_t){ k.v & (subrowsSize-1) };
    P(ret)->row2subrow[rowIndex.v] = subrowIndex;
    P(ret)->subrow2row[subrowIndex.v] = rowIndex;
  })

  rowIndexedSet_delete(s);
  DBG(DBG_LVL_DETAILED, "creation done.\n")
  return ret;
}


void subrowHashed_delete(subrowHashed_t s){
  DBG(DBG_LVL_MINIMAL, "delete\n")
  mem_free(P(s)->row2subrow);
  mem_free(P(s)->subrow2row);
  mem_free(P(s));
}
T__THREAD_SAFE bool subrowHashed_containsSubrow(subrowHashed_t s, subrowHash_t subrow){
  return P(s)->subrow2row[subrow.v].v != 0;
}
T__THREAD_SAFE subrowHash_t subrowHashed_row2subrow(subrowHashed_t s, rowHash_t row){
  subrowHash_t ret = P(s)->row2subrow[row.v];
  DBG(DBG_LVL_DETAILED, "subrowHashed_row2subrow, rowHash=0x%lX, subrowHash=0x%lX\n", (long) row.v, (long) ret.v)
  return ret;
}
T__THREAD_SAFE rowHash_t subrowHashed_subrow2row(subrowHashed_t s, subrowHash_t subrow){
  rowHash_t ret = P(s)->subrow2row[subrow.v];
  DBG(DBG_LVL_DETAILED, "subrowHashed_subrow2row, subrowHash=0x%lX, rowHash=0x%lX\n", (long) subrow.v, (long) ret.v)
  return ret;
}
T__THREAD_SAFE pow2size_t subrowHashed_numUniqueBySubrow(subrowHashed_t s){
  pow2size_t ret = P(s)->subrowsSize;
  DBG(DBG_LVL_DETAILED, "subrowHashed_numUniqueBySubrow, ret=%ld\n", (long) ret)
  return ret;
}
