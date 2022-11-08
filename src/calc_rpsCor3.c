//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <math.h>

#include "calc.h"
#include "calcUtils.h"
#include "wrapper.h"
#include "mem.h"



typedef struct{
  bool hasCorr;
} rowInfo_v_t;

T__THREAD_SAFE static void getInfo(__attribute__((unused)) void *getInfo_param, wire_t d, wire_t numIns, fixed_cell_t *transform, void *ret_info){
  rowInfo_v_t *ret = ret_info;

  for(size_t i = 1; i < (1ull << numIns); i++){
    if(transform[calcUtils_intExpandByD(d, i)] != 0){
      ret->hasCorr = 1;
      return;
    }
  }
  ret->hasCorr = 0;
}

void calc_rpsCor3(gadget_t *g, wire_t maxCoeff, double *ret_coeffs){
  double sdMax = 1 - ldexp(1.0, -g->numIns);

  rowInfo_generator_t w_gen = (rowInfo_generator_t){
    .hashTheTransforms = 0,
    .infoSize = sizeof(rowInfo_v_t),
    .getInfo_param = NULL,
    .getInfo = getInfo
  };
  wrapper_t w = wrapper_new(g, maxCoeff, -1, w_gen, "rpsCor3");

  pow2size_t subrowsSize = wrapper_subrowSize(w);
  bitArray_t isBySubrow = BITARRAY_CALLOC(subrowsSize, "rpsCor3");

  WRAPPER_ITERATE_SUBROWS(w, i, row, {
    ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, sub, {
      rowInfo_v_t *info = wrapper_getRowInfo(w, sub);
      if(info->hasCorr){
        bitArray_set(subrowsSize, isBySubrow, i.v);
        break;
      }
    })
  })

  wrapper_freeRowInfo(w);
  wrapper_startCalculatingCoefficients(w);

  memset(ret_coeffs, 0, sizeof(double) * (g->numTotProbes+1));
  BITARRAY_DEF_VAR(g->numTotOuts, row)
  row_first(g->numTotOuts, row);
  do{
    subrowHash_t subrowHash = wrapper_getRow2Subrow(w, row);
    bool is = bitArray_get(subrowsSize, isBySubrow, subrowHash.v);
    if(is)
      calcUtils_addTotProbeMulteplicity(ret_coeffs, sdMax, maxCoeff, g, row);
  }while(row_tryNext_probeLeqMaxCoeff(row, g->numOuts * g->d, g->numTotOuts, maxCoeff));

  mem_free(isBySubrow);
  wrapper_delete(w);

  for(wire_t i = maxCoeff+1; i <= g->numTotProbes; i++)
    ret_coeffs[i] = binomial(g->numTotProbes, i) * sdMax;
}
