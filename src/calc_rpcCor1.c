//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <time.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "wrapper.h"

// num safe input combinations I is (\sum_{i <= t} binom(d, i))**numIns
// this is < (\sum_i binom(d, i))**numIns = (2**d)**numIns = 1 << numMaskedIns

typedef struct{
  wire_t t;
} getInfo_t;

static inline int xor_col(size_t v1, size_t v2){
  size_t v = v1 & v2;
  wire_t ones = __builtin_popcountll(v);
  return (ones % 2 == 0) ? 1 : -1;
}

T__THREAD_SAFE static void getInfo(void *getInfo_param, wire_t d, wire_t numIns, fixed_cell_t *transform, void *ret_info){
  getInfo_t *p = getInfo_param;
  double *ret = ret_info;

  wire_t numMaskedIns = d * numIns;
  size_t numNoRndCols = 1ll << numMaskedIns;

  ITERATE_X_ACT(numMaskedIns, {
    ITERATE_II(d, numIns, p->t, {
      for(size_t i = 0; i < numNoRndCols; i++)
        if((i &~ ii) != 0)
          ret[ii_index + x * numNoRndCols] += xor_col(i, x) == 1 ? transform[i] : -transform[i];
    })
  })
}

static inline int xor_row(wire_t numTotOuts, bitArray_t v1, bitArray_t v2){
  return (bitArray_count1And(numTotOuts, v1, v2) % 2 == 0) ? 1 : -1;
}

void calc_rpcCor1(gadget_t *g, wire_t maxCoeff, wire_t t, double *ret_coeffs){
  getInfo_t getInfo_p = (getInfo_t){ t };
  wire_t numMaskedIns = g->d * g->numIns;
  size_t numNoRndCols = 1ll << numMaskedIns;

  rowInfo_generator_t w_gen = (rowInfo_generator_t){
    .hashTheTransforms = 1,
    .infoSize = sizeof(double) * numNoRndCols * numNoRndCols,
    .getInfo_param = &getInfo_p,
    .getInfo = getInfo
  };

  wrapper_t w = wrapper_new(g, maxCoeff, t, w_gen, "rpcSd");

  pow2size_t subrowsSize = wrapper_subrowSize(w);
  double *sdBySubrow = mem_calloc(sizeof(double), subrowsSize, "rpcSd's sdBySubrow");

  WRAPPER_ITERATE_SUBROWS(w, h, row, {
    wire_t rowCount1 = bitArray_count1(g->numTotOuts, row);
    double **infos = mem_calloc(sizeof(double*), 1ll << rowCount1, "rpcSd's subrows infos.");
    {
      size_t i = 0;
      ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, psi, {
        infos[i++] = wrapper_getRowInfo(w, psi);
      })
    }

    double min = DBL_MAX;
    ITERATE_II(g->d, g->numIns, t, {
      double max = 0.0;
      ITERATE_X_ACT(numMaskedIns, {
        double outer = 0.0;
        ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, o, {
          double inner = 0.0;
          size_t i = 0;
          ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, psi, {
            double v = infos[i][ii_index + x * numNoRndCols];
            inner += xor_row(g->numTotOuts, o, psi) == 1 ? v : -v;
            i++;
          })
          outer += ABS(inner);
        })
        max = MAX(max, outer);
      })

      max = ldexp(max,
        - g->numTotIns // fixed_cell_t is in fixed point notation. That is handled here.
        - 1 // a /2, see the formula
        - rowCount1 // 2**(- row_count1(row))  =  multeplicity * 2**(- numprobes)
      );
      double sdMax = 1 - ldexp(1.0, -g->numIns * g->d + COUNT_1(ii));
      max = MIN(sdMax, max);

      min = MIN(min, max);
    })
    sdBySubrow[h.v] = min;

    mem_free(infos);
  })

  wrapper_freeRowInfo(w);
  wrapper_startCalculatingCoefficients(w);

  memset(ret_coeffs, 0, sizeof(double) * (g->numTotProbes+1));
  BITARRAY_DEF_VAR(g->numTotOuts, out)
  row_first(g->numTotOuts, out);
  do{
    BITARRAY_DEF_VAR(g->numTotOuts, probe)
    row_first(g->numTotOuts, probe);

    double coeff_sum[g->numTotProbes+1];
    memset(coeff_sum, 0, sizeof(double) * (g->numTotProbes+1));
    do{
      BITARRAY_DEF_VAR(g->numTotOuts, row)
      bitArray_or(g->numTotOuts, out, probe, row);
      subrowHash_t subrowHash = wrapper_getRow2Subrow(w, row);
      double sd = sdBySubrow[subrowHash.v];
      calcUtils_addTotProbeMulteplicity(coeff_sum, sd, maxCoeff, g, probe);
    }while(row_tryNext_probeLeqMaxCoeff(probe, g->numOuts * g->d, g->numTotOuts, maxCoeff));

    calcUtils_coeffMaxIntoFirst(g->numTotProbes, ret_coeffs, coeff_sum);
  }while(row_tryNext_outLeqT(out, g->numOuts, g->d, t));

  mem_free(sdBySubrow);
  wrapper_delete(w);

  double sdMax = 1 - ldexp(1.0, -g->numIns * (g->d - t));
  for(wire_t i = maxCoeff+1; i <= g->numTotProbes; i++)
    ret_coeffs[i] = binomial(g->numTotProbes, i) * sdMax;
}
