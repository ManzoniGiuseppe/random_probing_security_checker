//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "calc.h"
#include "calcUtils.h"
#include "wrapper.h"
#include "probeComb.h"
#include "mem.h"


typedef struct{
  wire_t numTotIns;
} getInfo_t;

typedef struct{
  double sumCorr;
} rowInfo_v_t;

T__THREAD_SAFE static void getInfo(void *getInfo_param, wire_t d, wire_t numIns, fixed_cell_t *transform, void *ret_info){
  double sdMax = 1 - ldexp(1.0, -numIns);

  getInfo_t *p = getInfo_param;
  rowInfo_v_t *ret = ret_info;

  for(size_t i = 1; i < (1ull << numIns) && ret->sumCorr < sdMax; i++){
    fixed_cell_t it = transform[calcUtils_intExpandByD(d, i)];
    ret->sumCorr += ldexp(ABS(it),
      - p->numTotIns // fixed_cell_t is in fixed point notation. That is handled here.
      - 1  // a /2, see the formula
    );
  }
  if(ret->sumCorr >= sdMax)
    ret->sumCorr = sdMax;
}

void calc_rpsCor2(gadget_t *g, wire_t maxCoeff, double *ret_coeffs){
  double sdMax = 1 - ldexp(1.0, -g->numIns);

  getInfo_t getInfo_param = (getInfo_t){ g-> numTotIns };
  rowInfo_generator_t w_gen = (rowInfo_generator_t){
    .hashTheTransforms = 0,
    .infoSize = sizeof(rowInfo_v_t),
    .getInfo_param = &getInfo_param,
    .getInfo = getInfo
  };
  wrapper_t w = wrapper_new(g, maxCoeff, -1, w_gen, "rpsCor2");

  pow2size_t subrowsSize = wrapper_subrowSize(w);
  double *sumBySubrow = mem_calloc(sizeof(double), subrowsSize, "rpsCor2");

  WRAPPER_ITERATE_SUBROWS(w, i, row, {
    ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, sub, {
      rowInfo_v_t *info = wrapper_getRowInfo(w, sub);
      sumBySubrow[i.v] += info->sumCorr;
      if(sumBySubrow[i.v] >= sdMax)
        break;
    })
    if(sumBySubrow[i.v] >= sdMax)
      sumBySubrow[i.v] = sdMax;
  })

  wrapper_freeRowInfo(w);
  wrapper_startCalculatingCoefficients(w);

  memset(ret_coeffs, 0, sizeof(double) * (g->numTotProbes+1));
  ITERATE_OVER_PROBES(maxCoeff, g, probes, coeffNum, {
    subrowHash_t subrowHash;
    {
      BITARRAY_DEF_VAR(g->numTotOuts, highestRow)
      probeComb_getHighestRow(probes, g, highestRow);
      subrowHash = wrapper_getRow2Subrow(w, highestRow);
    }
    double sum = sumBySubrow[subrowHash.v];
    if(sum != 0.0){
      wire_t shift_by = probeComb_getRowMulteplicity(probes, g);
      sum = ldexp(sum, shift_by); // sum <<= shift_by
      if(sum >= sdMax)
        sum = sdMax;
    }

    ret_coeffs[coeffNum] += sum * probeComb_getProbesMulteplicity(probes, g);
  })

  mem_free(sumBySubrow);
  wrapper_delete(w);

  for(wire_t i = maxCoeff+1; i <= g->numTotProbes; i++)
    ret_coeffs[i] = binomial(g->numTotProbes, i) * sdMax;
}

