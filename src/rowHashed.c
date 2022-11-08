//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "types.h"
#include "hashSet.h"
#include "rowHashed.h"
#include "mem.h"


#define DBG_FILE "rowHashed"
#define DBG_LVL DBG_ROWHASHED


typedef struct{
  hashSet_t set;
  pow2size_t numRows;
  wire_t numTotOuts;
} rowHashed_storage_t;
#define P(pub)   ((rowHashed_storage_t *) ((rowHashed_t)(pub)).rowHashed)


rowHashed_t rowHashed_alloc(void *info, wire_t numTotOuts, bitArray_t first, bool (*tryNext)(void *info, bitArray_t next)){
  DBG(DBG_LVL_MINIMAL, "alloc\n")
  rowHashed_t ret = { mem_calloc(sizeof(rowHashed_storage_t), 1, "rowHashed main") };
  P(ret)->numTotOuts = numTotOuts;
  DBG(DBG_LVL_DETAILED, "adding rows...\n")
  P(ret)->set = hashSet_new(BITARRAY_I_BYTE_SIZE(numTotOuts), "rowHashed_alloc");
  {
    BITARRAY_DEF_VAR(numTotOuts, key)
    bitArray_copy(numTotOuts, first, key);
    do{
      hashSet_add(P(ret)->set, key);
    }while(tryNext(info, key));
  }

  P(ret)->numRows = hashSet_getNumElem(P(ret)->set);
  DBG(DBG_LVL_MINIMAL, "fill rate = %d%%, hash conflict rate = %d%%\n", (int)(hashSet_dbg_fill(P(ret)->set) * 100), (int)(hashSet_dbg_hashConflictRate(P(ret)->set) * 100))
  return ret;
}

void rowHashed_free(rowHashed_t s){
  hashSet_delete(P(s)->set);
  mem_free(P(s));
}

T__THREAD_SAFE bool rowHashed_contains(rowHashed_t s, bitArray_t row){ return hashSet_contains(P(s)->set, row); }
T__THREAD_SAFE wire_t rowHashed_numTotOuts(rowHashed_t s){ return P(s)->numTotOuts; }
bitArray_t rowHashed_get(rowHashed_t s, rowHash_t index){ return hashSet_getKeyR(P(s)->set, index.v); }
pow2size_t rowHashed_getSize(rowHashed_t s){ return P(s)->numRows; }

rowHash_t rowHashed_hash(rowHashed_t s, bitArray_t row){
  ON_DBG(DBG_LVL_TOFIX, {
    char str[2 + BITARRAY_I_BYTE_SIZE(P(s)->numTotOuts)*2 + 1];
    bitArray_toStr(P(s)->numTotOuts, row, str);
    if(!rowHashed_contains(s, row)) FAIL("rowHashed: missing row=%s\n", str)
  })

  hash_t ret = hashSet_getPos(P(s)->set, row);
  return (rowHash_t){ ret.v & (P(s)->numRows - 1) };
}

bool rowHashed_hasVal(rowHashed_t s, rowHash_t ret){
  void *key;
  return hashSet_hasVal(P(s)->set, ret.v, &key);
}
