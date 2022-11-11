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
  uint_least64_t arrayHasCoordWithCorr; // array of bits. each position x means: does exists a spectral coordinate with the x-th bit set that has correlation with the selected wire.
} rowInfo_v_t;

T__THREAD_SAFE static void getInfo(__attribute__((unused)) void *getInfo_param, wire_t d, wire_t numIns, fixed_cell_t *transform, void *ret_info){
  rowInfo_v_t *ret = ret_info;
  ret->arrayHasCoordWithCorr = 0;
  for(size_t i = 1; i < (1ull << (d*numIns)); i++)
    if(transform[i] != 0)
      ret->arrayHasCoordWithCorr |= i;
}

T__THREAD_SAFE static void iterateOverUniqueBySubrows(gadget_t *g, __attribute__((unused)) wire_t maxCoeff, wire_t t, wrapper_t w, int thread){
  double sdMax = 1 - ldexp(1.0, -g->numIns * (g->d - t));

  WRAPPER_ITERATE_SUBROWS(w, thread, i, row, {
    uint_fast64_t arrayHasCoordWithCorr = 0;
    ITERATE_OVER_SUB_ROWS(g->numTotOuts, row, sub, {
      rowInfo_v_t *info = wrapper_getRowInfo(w, sub);
      arrayHasCoordWithCorr |= info->arrayHasCoordWithCorr;
    })

    if(calcUtils_maxSharesIn(g->d, g->numIns, arrayHasCoordWithCorr) > t)
      wrapper_setSdOfRow(w, i, 0, sdMax);
  })
}

void calc_rpcCor2(gadget_t *g, wire_t maxCoeff, wire_t t, double *ret_coeffs){
  double sdMax = 1 - ldexp(1.0, -g->numIns * (g->d - t));

  if(g->d * g->numIns > 64) FAIL("rpcVraps, unsupported g->d * g->numIns > 64")

  rowInfo_generator_t w_gen = (rowInfo_generator_t){
    .hashTheTransforms = 0,
    .infoSize = sizeof(rowInfo_v_t),
    .getInfo_param = NULL,
    .getInfo = getInfo
  };

  wrapper_t w = wrapper_new(g, maxCoeff, t, w_gen, 1, iterateOverUniqueBySubrows, "rpcVraps");

  memset(ret_coeffs, 0, sizeof(double) * (g->numTotProbes+1));
  BITARRAY_DEF_VAR(g->numTotOuts, out)
  row_first(g->numTotOuts, out);
  do{
    if(row_maxShares(out, g->numOuts, g->d) != t) continue; // the max will be in the ones with most outputs
    double coeff_curr[g->numTotProbes+1];
    memset(coeff_curr, 0, sizeof(double) * (g->numTotProbes+1));

    BITARRAY_DEF_VAR(g->numTotOuts, probe)
    row_first(g->numTotOuts, probe);
    do{
      BITARRAY_DEF_VAR(g->numTotOuts, row)
      bitArray_or(g->numTotOuts, out, probe, row);
      subrowHash_t subrowHash = wrapper_getRow2Subrow(w, row);
      double sd = wrapper_getSdOfRow(w, subrowHash, 0);
      calcUtils_addTotProbeMulteplicity(coeff_curr, sd, maxCoeff, g, probe);
    }while(row_tryNext_probeLeqMaxCoeff(probe, g->numOuts * g->d, g->numTotOuts, maxCoeff));

    calcUtils_coeffMaxIntoFirst(g->numTotProbes, ret_coeffs, coeff_curr);
  }while(row_tryNext_outLeqT(out, g->numOuts, g->d, t));

  wrapper_delete(w);

  for(wire_t i = maxCoeff+1; i <= g->numTotProbes; i++)
    ret_coeffs[i] = binomial(g->numTotProbes, i) * sdMax;
}
