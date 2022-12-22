//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <string.h>

#include "mem.h"
#include "rowInfo.h"
#include "rowIndexedSet.h"
#include "multithread.h"


#define DBG_FILE "rowInfo"
#define DBG_LVL DBG_ROWINFO


typedef struct {
  transformGenerator_t tg;
  rowHashed_t rows;
  rowInfo_generator_t gen;
  wire_t d;
  size_t numberSize;
  wire_t numIns;
  rowIndexedSet_t ret;
  multithread_mtx_t *mtx;
} info_t;


void fn_direct(void *info){
  info_t *p = (info_t*)info;
  transformGenerator_t tg = p->tg;
  rowHashed_t rows = p->rows;
  rowInfo_generator_t gen = p->gen;
  wire_t d = p->d;
  size_t numberSize = p->numberSize;
  wire_t numIns = p->numIns;
  rowIndexedSet_t ret = p->ret;
  multithread_mtx_t *mtx = p->mtx;

  ROWHASHED_ITERATE_TH(rows, index, {
    DBG(DBG_LVL_DETAILED, "index %d, obtaining transform...\n", index);
    size_t numNoRndCols = 1ll << d*numIns;
    number_t transform[numNoRndCols * numberSize];
    memset(transform, 0, sizeof(number_t) * numNoRndCols * numberSize);
    transformGenerator_getTranform(tg, index, transform);

    DBG(DBG_LVL_DETAILED, "obtaining info from transform...\n");
    uint8_t info[gen.infoSize];
    memset(info, 0, gen.infoSize);
    gen.getInfo(gen.getInfo_param, d, numIns, numberSize, transform, info);

    DBG(DBG_LVL_DETAILED, "adding info.\n");
    if(!multithread_mtx_lock(mtx)) FAIL("rowInfo_build's fn_direct fail to lock\n")
    rowIndexedSet_add(ret, index, info);
    if(!multithread_mtx_unlock(mtx)) FAIL("rowInfo_build's fn_direct fail to unlock\n")
  })
}

void fn_hashTransform(void *info){
  info_t *p = (info_t*)info;
  transformGenerator_t tg = p->tg;
  rowHashed_t rows = p->rows;
  rowInfo_generator_t gen = p->gen;
  wire_t d = p->d;
  size_t numberSize = p->numberSize;
  wire_t numIns = p->numIns;
  rowIndexedSet_t ret = p->ret;
  multithread_mtx_t *mtx = p->mtx;

  ROWHASHED_ITERATE_TH(rows, index, {
    DBG(DBG_LVL_DETAILED, "index %d, obtaining transform...\n", index);
    size_t numNoRndCols = 1ll << d*numIns;
    number_t transform[numNoRndCols * numberSize];
    memset(transform, 0, sizeof(number_t) * numNoRndCols * numberSize);
    transformGenerator_getTranform(tg, index, transform);

    for(size_t i = 0; i < numNoRndCols; i++){
      if(!bitArray_get(numNoRndCols, gen.hashTheTransforms_usingPositions, i)){
        for(size_t v = 0; v < numberSize; v++){
          transform[i * p->numberSize + v] = 0;
        }
      }
    }

    DBG(DBG_LVL_DETAILED, "adding transform.\n");
    if(!multithread_mtx_lock(mtx)) FAIL("rowInfo_build's fn_hashTransform fail to lock\n")
    rowIndexedSet_add(ret, index, transform);
    if(!multithread_mtx_unlock(mtx)) FAIL("rowInfo_build's fn_hashTransform fail to unlock\n")
  })
}

typedef struct{
  void **tr2info;
  rowInfo_generator_t gen;
  rowIndexedSet_t tr;
  wire_t d;
  size_t numberSize;
  wire_t numIns;
} info_tr2info_t;

void fn_tr2info(void *info){
  info_tr2info_t *p = (info_tr2info_t*)info;

  int thread = multithread_thr_getId();
  if(thread < 0) thread = 0;

  rowInfo_generator_t gen = p->gen;

  ROWINDEXEDSET_ITERATE_TH(p->tr, baseH, reducedH, transform, {
    void *info = p->tr2info[thread] + (reducedH - baseH) * gen.infoSize;
    gen.getInfo(gen.getInfo_param, p->d, p->numIns, p->numberSize, transform, info);
  })
}


typedef struct {
  size_t infoSize;
  bool isDirect;

  rowIndexedSet_t direct;

  pow2size_t tr_size;
  size_t tr_baseSize;
  rowIndexedSet_t tr;
  void *tr2info[NUM_THREADS > 0 ? NUM_THREADS : 1];
} rowInfo_storage_t;
#define P(pub)   ((rowInfo_storage_t *) ((rowInfo_t)(pub)).rowInfo)


T__THREAD_SAFE rowInfo_t rowInfo_build(transformGenerator_t tg, rowHashed_t rows, rowInfo_generator_t gen, wire_t d, wire_t numIns, wire_t numRnds){
  rowInfo_t ret = { mem_calloc(sizeof(rowInfo_storage_t), 1, "rowInfo's main struct") };
  P(ret)->infoSize = gen.infoSize;
  pow2size_t size = rowHashed_getSize(rows);

  multithread_mtx_t mtx;
  info_t info = (info_t){
    .tg = tg,
    .rows = rows,
    .gen = gen,
    .d = d,
    .numberSize = NUMBER_SIZE((d*numIns+numRnds)+3),
    .numIns = numIns,
    .mtx = &mtx
  };

  if(!multithread_mtx_init(&mtx)) FAIL("rowInfo_build fail to init lock\n")

  if(!gen.hashTheTransforms_usingPositions){
    DBG(DBG_LVL_MINIMAL, "new with size=%d\n", size);
    P(ret)->direct = rowIndexedSet_new(size, gen.infoSize, "rowInfo's store");
    P(ret)->isDirect = 1;

    info.ret = P(ret)->direct;
    multithread_thr_parallel(&info, fn_direct);
  }else{
    DBG(DBG_LVL_MINIMAL, "hashing transforms\n");
    P(ret)->tr = rowIndexedSet_new(size, sizeof(number_t) * (1ll << d*numIns) * info.numberSize, "rowInfo's transform");
    P(ret)->isDirect = 0;
    info.ret = P(ret)->tr;
    multithread_thr_parallel(&info, fn_hashTransform);

    DBG(DBG_LVL_MINIMAL, "getting infos\n");
    P(ret)->tr_size = rowIndexedSet_currSize(P(ret)->tr);
#if NUM_THREADS > 0
    P(ret)->tr_baseSize = P(ret)->tr_size / NUM_THREADS;
    for(int i = 0; i < NUM_THREADS; i++)
      P(ret)->tr2info[i] = mem_calloc(gen.infoSize, P(ret)->tr_size / NUM_THREADS + NUM_THREADS, "rowInfo's transform to info");
#else
    P(ret)->tr_baseSize = P(ret)->tr_size;
    P(ret)->tr2info[0] = mem_calloc(gen.infoSize, P(ret)->tr_size, "rowInfo's transform to info");
#endif

    info_tr2info_t info_tr2info = (info_tr2info_t){
      .tr = P(ret)->tr,
      .tr2info = P(ret)->tr2info,
      .gen = gen,
      .d = d,
      .numIns = numIns,
      .numberSize = info.numberSize
    };

    multithread_thr_parallel(&info_tr2info, fn_tr2info);
  }

  DBG(DBG_LVL_MINIMAL, "new completed\n");

  multithread_mtx_destroy(&mtx);
  return ret;
}


void rowInfo_delete(rowInfo_t s){
  if(P(s)->isDirect){
    rowIndexedSet_delete(P(s)->direct);
  }else{
    rowIndexedSet_delete(P(s)->tr);
    for(int i = 0; i < (NUM_THREADS > 0 ? NUM_THREADS : 1); i++)
      mem_free(P(s)->tr2info[i]);
  }
  mem_free(P(s));
}

T__THREAD_SAFE T__LEAKS void * rowInfo_row2info(rowInfo_t s, rowHash_t row){
  if(P(s)->isDirect){
    hash_t infoHash = rowIndexedSet_row2valIndex(P(s)->direct, row);
    return rowIndexedSet_getVal(P(s)->direct, infoHash);
  }else{
    hash_t infoHash = rowIndexedSet_row2valIndex(P(s)->tr, row);
    size_t index = infoHash.v & ( rowIndexedSet_currSize(P(s)->tr)-1 );

#if NUM_THREADS > 0
    int i = index / P(s)->tr_baseSize;
    if(i == NUM_THREADS)
      i--;

    return P(s)->tr2info[i] + (index - i * P(s)->tr_baseSize) * P(s)->infoSize;
#else
    return P(s)->tr2info[0] + index * P(s)->infoSize;
#endif
  }
}

T__THREAD_SAFE hash_t rowInfo_row2hashInfo(rowInfo_t s, rowHash_t row){
  if(P(s)->isDirect){
    return rowIndexedSet_row2valIndex(P(s)->direct, row);
  }else{
    return rowIndexedSet_row2valIndex(P(s)->tr, row);
  }
}
