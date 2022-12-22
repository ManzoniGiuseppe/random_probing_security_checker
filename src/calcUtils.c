//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "calcUtils.h"
#include "probeComb.h"
#include "mem.h"
#include "row.h"


// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
size_t calcUtils_intExpandByD(wire_t d, size_t val){
  if(val == 0) return 0;
  size_t bit = MASK_OF(d);
  size_t ret = 0;
  for(wire_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << (i*d);
  }
  return ret;
}

void calcUtils_addTotProbeMulteplicity(double *addToCoeffs, double toAdd, wire_t maxCoeff, gadget_t *g, bitArray_t highest_row){
  ITERATE_OVER_PROBES_WHOSE_IMAGE_IS(maxCoeff, g, highest_row, probeComb, coeffNum, {
    double mul = probeComb_getProbesMulteplicity(probeComb, g);
    addToCoeffs[coeffNum] += mul * toAdd;
  })
}

wire_t calcUtils_maxSharesIn(wire_t d, wire_t numIns, size_t value){
  wire_t ret = 0;
  size_t mask = MASK_OF(d);
  for(wire_t i = 0; i < numIns; i++){
    unsigned v = __builtin_popcountll(value & (mask << (i*d)));
    ret = v > ret ? v : ret;
  }
  return ret;
}

static double calculateF(wire_t maxCoeff, double *coeffs, double p){
  double ret = 0.0;
  double pi;
  size_t i;
  for(pi = 1.0, i = 0; i <= maxCoeff; i++, pi *= p){
    ret += coeffs[i] * pi;
  }
  return MIN(1.0, ret); // f is a probability, so it's in [0,1]
}

void calcUtils_coeffMaxIntoFirst(wire_t maxCoeff, double *v1, double *v2){
  // check the coefficients
  bool all_ge = 1;
  bool all_le = 1;
  for(wire_t i = 0; i <= maxCoeff; i++){
    if(v1[i] < v2[i]) all_ge = 0;
    if(v1[i] > v2[i]) all_le = 0;
  }

  if(all_ge) return;
  if(all_le){
    for(wire_t i = 0; i <= maxCoeff; i++)
      v1[i] = v2[i];
    return;
  }

  // check again by sempling the functions, to see if all the coeff are actually expressed
  all_ge = all_le = 1;
  for(double p = 0.0; p <= 1.0; p+=FN_CMP_STEP){
    double cf_v1 = calculateF(maxCoeff, v1, p);
    double cf_v2 = calculateF(maxCoeff, v2, p);
    if(cf_v1 < cf_v2) all_ge = 0;
    if(cf_v1 > cf_v2) all_le = 0;
  }

  if(all_ge) return;
  if(all_le){
    for(wire_t i = 0; i <= maxCoeff; i++)
      v1[i] = v2[i];
    return;
  }

  // mixed
  for(wire_t i = 0; i <= maxCoeff; i++)
    v1[i] = MAX(v1[i], v2[i]);
}
