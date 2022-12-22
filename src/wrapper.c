//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <time.h>

#include "wrapper.h"
#include "rowHashed.h"
#include "rowInfo.h"
#include "subrowHashed.h"
#include "bitArray.h"
#include "mem.h"
#include "multithread.h"

#define DBG_FILE "wrapper"
#define DBG_LVL DBG_WRAPPER


//#define NUM_THREADS



typedef struct{
  wire_t numOuts;
  wire_t d;
  wire_t numTotOuts;
  wire_t maxCoeff;
  wire_t t;
} tryNext_t;

static bool tryNext(void *info, bitArray_t next){
  tryNext_t *s = info;
  if(s->t != 0)
    if(row_tryNext_outLeqT(next, s->numOuts, s->d, s->t)) return 1;
  return row_tryNext_probeLeqMaxCoeff(next, s->numOuts * s->d, s->numTotOuts, s->maxCoeff);
}

static hash_t row2info(void* infoR2I, rowHash_t row){
  return rowInfo_row2hashInfo((rowInfo_t){ infoR2I }, row);
}

typedef struct{
  rowHashed_t rows;
  rowInfo_t rowInfo;
  subrowHashed_t subrows;
  multithread_double_t *sdBySubrow;
  int numAlternativeSd;
  const char *what;
  time_t last;
} wrapper_storage_t;
#define P(pub)   ((wrapper_storage_t *) ((wrapper_t)(pub)).wrapper)


static void printTimediff(wrapper_t w){
  ON_DBG(DBG_LVL_MINIMAL, {
    time_t curr;
    time(&curr);
    DBG(DBG_LVL_MINIMAL, "time: %f\n", difftime(curr, P(w)->last))
    P(w)->last = curr;
  })
}



typedef struct{
  gadget_t *g;
  wire_t maxCoeff;
  wire_t t;
  wrapper_t w;
  T__THREAD_SAFE void (*iterateOverUniqueBySubrows)(gadget_t *g, wire_t maxCoeff, wire_t t, wrapper_t w);
} iterateOverUniqueBySubrows_t;

void iterateOverUniqueBySubrows_fn(void *p){
  iterateOverUniqueBySubrows_t *info = (iterateOverUniqueBySubrows_t *)p;
  info->iterateOverUniqueBySubrows(info->g, info->maxCoeff, info->t, info->w);
}

wrapper_t wrapper_new(
  gadget_t *g,
  wire_t maxCoeff,
  wire_t t,  // >= 0 for RPC, 0 for RPS
  rowInfo_generator_t gen,
  int numAlternativeSd,
  T__THREAD_SAFE void (*iterateOverUniqueBySubrows)(gadget_t *g, wire_t maxCoeff, wire_t t, wrapper_t w),
  const char *what
){
  wrapper_t ret = { mem_calloc(sizeof(wrapper_storage_t),1,"what") };
  time(&P(ret)->last);

  printf("%s: 0/6: hash all rows\n", what);
  tryNext_t tryNext_info = (tryNext_t){
    .d = g->d,
    .numOuts = g->numOuts,
    .numTotOuts = g->numTotOuts,
    .maxCoeff = maxCoeff,
    .t = t
  };
  BITARRAY_DEF_VAR(g->numTotOuts, firstRow)
  row_first(g->numTotOuts, firstRow);
  bitArray_iterator_t itRows = (bitArray_iterator_t){
    .info = &tryNext_info,
    .first = firstRow,
    .tryNext = tryNext
  };
  P(ret)->rows = rowHashed_alloc(itRows, g->numTotOuts);
  printTimediff(ret);

  printf("%s: 1/6: initial calculation of transform of all rows\n", what);
  transformGenerator_t tg = transformGenerator_alloc(P(ret)->rows, g);
  printTimediff(ret);

  printf("%s: 2/6: calculate transform and row-only info\n", what);
  P(ret)->rowInfo = rowInfo_build(tg, P(ret)->rows, gen, g->d, g->numIns, g->numRnds);
  transformGenerator_free(tg);
  printTimediff(ret);

  printf("%s: 3/6: calculate which rows are unique by the info/values of their subrows\n", what);
  P(ret)->subrows = subrowHashed_new(P(ret)->rows, itRows, P(ret)->rowInfo.rowInfo, row2info);
  printTimediff(ret);

  printf("%s: 4/6: calculate the SD for each row unique by the info/values of their subrows\n", what);
  P(ret)->what = what;
  P(ret)->numAlternativeSd = numAlternativeSd;

  pow2size_t subrowsSize = subrowHashed_numUniqueBySubrow(P(ret)->subrows);
  P(ret)->sdBySubrow = mem_calloc(sizeof(multithread_double_t), subrowsSize * numAlternativeSd, "rpcSd's sdBySubrow");
  #if NUM_THREADS > 0
    for(size_t i = 0; i < subrowsSize * numAlternativeSd; i++)
      multithread_double_init(P(ret)->sdBySubrow + i, 0.0);
  #endif

  iterateOverUniqueBySubrows_t info;
  info.g = g;
  info.maxCoeff = maxCoeff;
  info.t = t;
  info.w = ret;
  info.iterateOverUniqueBySubrows = iterateOverUniqueBySubrows;

  multithread_thr_parallel(&info, iterateOverUniqueBySubrows_fn); // re-entrant.

  rowInfo_delete(P(ret)->rowInfo);
  P(ret)->rowInfo.rowInfo = NULL;
  printTimediff(ret);

  printf("%s: 5/6: calculate the coefficient\n", what);
  return ret;
}

pow2size_t wrapper_subrowSize(wrapper_t w){ return subrowHashed_numUniqueBySubrow(P(w)->subrows); }
T__THREAD_SAFE bool wrapper_containsSubrow(wrapper_t w, subrowHash_t subrow){ return subrowHashed_containsSubrow(P(w)->subrows, subrow); }

bitArray_t wrapper_getSubrow2Row(wrapper_t w, subrowHash_t subrow){
  DBG(DBG_LVL_DETAILED, "%s, getSubrow2Row, subrow=%lld\n", P(w)->what, (long long) subrow.v)
  rowHash_t row = subrowHashed_subrow2row(P(w)->subrows, subrow);
  return rowHashed_get(P(w)->rows, row);
}

void *wrapper_getRowInfo(wrapper_t w, bitArray_t sub){
  DBG(DBG_LVL_DETAILED, "%s, getRowInfo\n", P(w)->what)
  rowHash_t subHash = rowHashed_hash(P(w)->rows, sub);
  return rowInfo_row2info(P(w)->rowInfo, subHash);
}
T__THREAD_SAFE double wrapper_getSdOfRow(wrapper_t w, __attribute__((unused)) subrowHash_t subrowHash, int alt){
  int numAlt = P(w)->numAlternativeSd;
  ON_DBG(DBG_LVL_TOFIX, {
    if(alt >= numAlt) FAIL("wrapper: wrapper_getSdOfRow, alt=%d numAlternativeSd=%d\n", alt, numAlt)
  })

  return multithread_double_get(P(w)->sdBySubrow + (subrowHash.v * numAlt + alt), MULTITHREAD_SYNC_VAL);
}
T__THREAD_SAFE void wrapper_setSdOfRow(wrapper_t w, subrowHash_t subrowHash, int alt, double sd){
  int numAlt = P(w)->numAlternativeSd;
  ON_DBG(DBG_LVL_TOFIX, {
    if(alt >= numAlt) FAIL("wrapper: wrapper_getSdOfRow, alt=%d numAlternativeSd=%d\n", alt, numAlt)
  })

  multithread_double_set(P(w)->sdBySubrow + (subrowHash.v * numAlt + alt), sd, MULTITHREAD_SYNC_VAL);
}

subrowHash_t wrapper_getRow2Subrow(wrapper_t w, bitArray_t row){
  DBG(DBG_LVL_DETAILED, "%s, getRow2Subrow\n", P(w)->what)
  rowHash_t hash = rowHashed_hash(P(w)->rows, row);
  DBG(DBG_LVL_MAX, "%s, getRow2Subrow, rowHash=%llX\n", P(w)->what, (long long) hash.v)
  return subrowHashed_row2subrow(P(w)->subrows, hash);
}

void wrapper_delete(wrapper_t w){
  printTimediff(w);
  printf("%s: 6/6: done\n", P(w)->what);

  rowHashed_free(P(w)->rows);
  subrowHashed_delete(P(w)->subrows);
  mem_free(P(w)->sdBySubrow);
  mem_free(P(w));
}
