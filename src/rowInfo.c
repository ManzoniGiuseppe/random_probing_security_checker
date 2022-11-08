//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <string.h>

#include "mem.h"
#include "rowInfo.h"
#include "rowIndexedSet.h"


#define DBG_FILE "rowInfo"
#define DBG_LVL DBG_ROWINFO


T__THREAD_SAFE rowIndexedSet_t rowInfo_build(transformGenerator_t tg, rowHashed_t rows, rowInfo_generator_t gen, wire_t d, wire_t numIns, wire_t numRnds){
  pow2size_t size = rowHashed_getSize(rows);
  DBG(DBG_LVL_MINIMAL, "new with size=%d\n", size);
  rowIndexedSet_t s = rowIndexedSet_new(size, gen.infoSize, "rowInfo's store");

  if(!gen.hashTheTransforms){
    ROWHASHED_ITERATE(rows, index, {
      DBG(DBG_LVL_DETAILED, "index %d, obtaining transform...\n", index);
      fixed_cell_t transform[1ll << d*numIns];
      memset(transform, 0, d*numIns);
      transformGenerator_getTranform(tg, index, d*numIns, numRnds, transform);

      DBG(DBG_LVL_DETAILED, "obtaining info from transform...\n");
      uint8_t info[gen.infoSize];
      memset(info, 0, gen.infoSize);
      gen.getInfo(gen.getInfo_param, d, numIns, transform, info);

      DBG(DBG_LVL_DETAILED, "adding info.\n");
      rowIndexedSet_add(s, index, info);
    })
  }else{
    rowIndexedSet_t tr = rowIndexedSet_new(size, sizeof(fixed_cell_t) * (1ll << d*numIns), "rowInfo's transform");

    ROWHASHED_ITERATE(rows, index, {
      DBG(DBG_LVL_DETAILED, "index %d, obtaining transform...\n", index);
      fixed_cell_t transform[1ll << d*numIns];
      memset(transform, 0, sizeof(fixed_cell_t) * (1ll << d*numIns));
      transformGenerator_getTranform(tg, index, d*numIns, numRnds, transform);

      DBG(DBG_LVL_DETAILED, "adding transform.\n");
      rowIndexedSet_add(tr, index, transform);
    })

    pow2size_t tr_size = rowIndexedSet_currSize(tr);
    void *tr2info = mem_calloc(gen.infoSize, tr_size, "rowInfo's transform to info");

    ROWINDEXEDSET_ITERATE(tr, reducedH, transform, {
      void *info = tr2info + reducedH * gen.infoSize;
      gen.getInfo(gen.getInfo_param, d, numIns, transform, info);
    })

    ROWHASHED_ITERATE(rows, index, {
      hash_t h = rowIndexedSet_row2valIndex(tr, index);
      reducedHash_t reducedH = h.v & (tr_size - 1);
      void *info = tr2info + reducedH * gen.infoSize;
      rowIndexedSet_add(s, index, info);
    })

    rowIndexedSet_delete(tr);
    mem_free(tr2info);
  }
  return s;
}
