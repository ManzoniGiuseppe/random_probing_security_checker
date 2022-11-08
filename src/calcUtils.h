//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _CALC_UTILS_H_
#define _CALC_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "row.h"
#include "gadget.h"



// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
size_t calcUtils_intExpandByD(wire_t d, size_t val);

void calcUtils_addTotProbeMulteplicity(double *addToCoeffs, double toAdd, wire_t maxCoeff, gadget_t *g, bitArray_t highest_row);

wire_t calcUtils_maxSharesIn(wire_t d, wire_t numIns, size_t value);

void calcUtils_coeffMaxIntoFirst(wire_t numTotProbes, double *coeff1AndRet, double *coeff2);


// additional var 'ii', 'ii_index'
#define ITERATE_II(d, numIns, t, code)  { \
    int ii_index = 0;  \
    for(size_t ii = 0; ii < (1ull << (d*numIns)); ii++){\
      if(calcUtils_maxSharesIn(d, numIns, ii) <= (t)){ \
        { code }  \
        ii_index++; \
      } \
    } \
  }

// additional var 'x'
#define ITERATE_X_UND(numIns, code)  { \
    for(size_t x = 0; x < (1ull << (numIns)); x++){ \
      { code }  \
    } \
  }

// additional var 'x'
#define ITERATE_X_ACT(numMaskedIns, code)  { \
    for(size_t x = 0; x < (1ull << (numMaskedIns)); x++){ \
      { code }  \
    } \
  }




#endif // _CALC_UTILS_H_
