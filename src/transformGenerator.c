//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "transformGenerator.h"
#include "hashMap.h"
#include "bdd.h"
#include "mem.h"


#define DBG_FILE  "transformGenerator"
#define DBG_LVL DBG_TRANSFORMGENERATOR


static void fnGenerator(bdd_t bdd, bdd_fn_t *x, bdd_fn_t *ret, gadget_t *g){
  for(wire_t i = 0; i < g->numTotOuts; i++){
    DBG(DBG_LVL_DETAILED, "Executing gadget, operation %d.\n", i);
    gadget_fnOperation_t op = g->op[i];

    if(op.operation == GADGET_IN){
      ret[op.outWire] = x[op.inWire[0]];
      continue;
    }

    bdd_fn_t v0 = ret[op.inWire[0]];
    if(op.operation == GADGET_NOT){
      ret[op.outWire] = bdd_op_not(bdd, v0);
      continue;
    }

    bdd_fn_t v1 = ret[op.inWire[1]];
    switch(op.operation){
      case GADGET_BIN_NOR: ret[op.outWire] = bdd_op_and(bdd, bdd_op_not(bdd, v0), bdd_op_not(bdd, v1)); break;
      case GADGET_BIN_NIMPLY: ret[op.outWire] = bdd_op_and(bdd, v0, bdd_op_not(bdd, v1)); break;
      case GADGET_BIN_XOR: ret[op.outWire] = bdd_op_xor(bdd, v0, v1); break;
      case GADGET_BIN_NAND: ret[op.outWire] = bdd_op_not(bdd, bdd_op_and(bdd, v0, v1)); break;
      case GADGET_BIN_AND: ret[op.outWire] = bdd_op_and(bdd, v0, v1); break;
      case GADGET_BIN_XNOR: ret[op.outWire] = bdd_op_not(bdd, bdd_op_xor(bdd, v0, v1)); break;
      case GADGET_BIN_IMPLY: ret[op.outWire] = bdd_op_not(bdd, bdd_op_and(bdd, v0, bdd_op_not(bdd, v1))); break;
      case GADGET_BIN_OR: ret[op.outWire] = bdd_op_not(bdd, bdd_op_and(bdd, bdd_op_not(bdd, v0), bdd_op_not(bdd, v1))); break;
    }
  }
}


typedef struct {
  bdd_fn_t *rowIndex2bdd;
  bdd_t bddStorage;
  bdd_sumCached_t bddCached;
  pow2size_t rowIndex2bdd_size;
} transformGenerator_storage_t;
#define P(pub)   ((transformGenerator_storage_t *) ((transformGenerator_t)(pub)).transformGenerator)

transformGenerator_t transformGenerator_alloc(rowHashed_t rows, gadget_t *g){
  pow2size_t rowSize = rowHashed_getSize(rows);

  DBG(DBG_LVL_MINIMAL, "new with rowSize=%d.\n", rowSize);
  transformGenerator_t ret = { mem_calloc(sizeof(transformGenerator_storage_t), 1, "transformGenerator_alloc's main struct") };
  P(ret)->bddStorage = bdd_storage_alloc();
  P(ret)->bddCached = bdd_sumCached_new(P(ret)->bddStorage, g->numTotIns, g->numIns * g->d);
  P(ret)->rowIndex2bdd = mem_calloc(sizeof(bdd_fn_t), rowSize, "transformGenerator_alloc's rowIndex2bdd");
  P(ret)->rowIndex2bdd_size = rowSize;

  DBG(DBG_LVL_DETAILED, "gettng bdd of return wires.\n");
  bdd_fn_t core[g->numTotOuts];
  {
    bdd_fn_t x[g->numTotIns];
    for(wire_t i = 0; i < g->numTotIns; i++){
      x[i] = bdd_val_single(P(ret)->bddStorage, i);
    }
    fnGenerator(P(ret)->bddStorage, x, core, g);
  }

  DBG(DBG_LVL_DETAILED, "saving bdds of all used rows.\n");
  wire_t numTotOuts = rowHashed_numTotOuts(rows);
  ROWHASHED_ITERATE(rows, index, {
    bitArray_t row = rowHashed_get(rows, index);

    bdd_fn_t rowFn = bdd_val_const(P(ret)->bddStorage, 0);

    ROW_ITERATE_OVER_ONES(numTotOuts, row, i, {
      rowFn = bdd_op_xor(P(ret)->bddStorage, rowFn, core[i]);
    })

    P(ret)->rowIndex2bdd[index.v] = rowFn;
  })
  return ret;
}

void transformGenerator_free(transformGenerator_t s){
  DBG(DBG_LVL_MINIMAL, "freeing\n");
  mem_free(P(s)->rowIndex2bdd);
  bdd_storage_free(P(s)->bddStorage);
  bdd_sumCached_delete(P(s)->bddCached);
  mem_free(P(s));
}




T__THREAD_SAFE void transformGenerator_getTranform(transformGenerator_t s, rowHash_t row, number_t *ret){ // ret contains  1ll << numMaskedIns  blocks of  NUM_SIZE(numTotIns+3)  num_t
  bdd_sumCached_transform(P(s)->bddCached, P(s)->rowIndex2bdd[row.v], ret);
}
