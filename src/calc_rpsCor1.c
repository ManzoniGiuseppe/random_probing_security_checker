//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "calc.h"
#include "calcUtils.h"
#include "wrapper.h"
#include "mem.h"


static inline int xor_col(size_t v1, size_t v2){
  size_t v = v1 & v2;
  wire_t ones = __builtin_popcountll(v);
  return (ones % 2 == 0) ? 1 : -1;
}

T__THREAD_SAFE static void getInfo(__attribute__((unused)) void *getInfo_param, wire_t d, wire_t numIns, fixed_cell_t *transform, void *ret_info){
  double *ret = ret_info;

  ITERATE_X_UND(numIns, {
    for(size_t i = 1; i < (1ull << numIns); i++){ // 0 excluded
      double v = transform[calcUtils_intExpandByD(d, i)];
      ret[x] += xor_col(i, x) == 1 ? v : -v;
    }
  })
}

static inline int xor_row(wire_t numTotOuts, bitArray_t v1, bitArray_t v2){
  return (bitArray_count1And(numTotOuts, v1, v2) % 2 == 0) ? 1 : -1;
}

void calc_rpsCor1(gadget_t *g, wire_t maxCoeff, double *ret_coeffs){
  double sdMax = 1 - ldexp(1.0, -g->numIns);

  size_t numCols = 1ll << g->numIns;
  rowInfo_generator_t w_gen = (rowInfo_generator_t){
    .hashTheTransforms = 1,
    .infoSize = sizeof(double) * numCols,
    .getInfo_param = NULL,
    .getInfo = getInfo
  };
  wrapper_t w = wrapper_new(g, maxCoeff, -1, w_gen, "rpsCor1");

  pow2size_t subrowsSize = wrapper_subrowSize(w);
  double *sdBySubrow = mem_calloc(sizeof(double), subrowsSize * numCols, "rpsCor1");

  WRAPPER_ITERATE_SUBROWS(w, h, row, {
    wire_t rowCount1 = bitArray_count1(g->numTotOuts, row);
    double **infos = mem_calloc(sizeof(double*), 1ll << rowCount1, "rpsCor1's subrows infos.");
    {
      size_t i = 0;
      ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, omega, {
        infos[i++] = wrapper_getRowInfo(w, omega);
      })
    }

    ITERATE_X_UND(g->numIns, {
      double outer = 0.0;
      ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, o, {
        double inner = 0.0;
        size_t i = 0;
        ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, omega, {
          inner += xor_row(g->numTotOuts, o, omega) == 1 ? infos[i][x] : -infos[i][x];
          i++;
        })
        outer += ABS(inner);
      })
      double ret = ldexp(outer,
        - g->numTotIns // fixed_cell_t is in fixed point notation. That is handled here.
        - 1 // a /2, see the formula
        - bitArray_count1(g->numTotOuts, row) // 2 ** - row_count1(row)  =  multeplicity * 2** - numprobes
      );
      sdBySubrow[h.v * numCols + x] = MIN(sdMax, ret);
    })

    mem_free(infos);
  })

  wrapper_freeRowInfo(w);
  wrapper_startCalculatingCoefficients(w);

  memset(ret_coeffs, 0, sizeof(double) * (g->numTotProbes+1));
  for(size_t x = 0; x < 1ull<<g->numIns; x++){
    double coeff_sum[g->numTotProbes+1];
    memset(coeff_sum, 0, sizeof(double) * (g->numTotProbes+1));

    BITARRAY_DEF_VAR(g->numTotOuts, probe)
    row_first(g->numTotOuts, probe);
    do{
      subrowHash_t subrowHash = wrapper_getRow2Subrow(w, probe);
      double sd = sdBySubrow[subrowHash.v * numCols + x];
      calcUtils_addTotProbeMulteplicity(coeff_sum, sd, maxCoeff, g, probe);
    }while(row_tryNext_probeLeqMaxCoeff(probe, g->d * g->numOuts, g->numTotOuts, maxCoeff));

    calcUtils_coeffMaxIntoFirst(g->numTotProbes, ret_coeffs, coeff_sum);
  }

  mem_free(sdBySubrow);
  wrapper_delete(w);

  for(wire_t i = maxCoeff+1; i <= g->numTotProbes; i++)
    ret_coeffs[i] = binomial(g->numTotProbes, i) * sdMax;
}
