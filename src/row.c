//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"
#include "row.h"




// -- getRow

wire_t row_maxShares(bitArray_t r, wire_t numOuts, wire_t d){
  wire_t max = 0;
  uint64_t outputs = bitArray_lowest_get(numOuts*d, r, numOuts*d);
  for(wire_t o = 0; o < numOuts; o++){
    wire_t val = COUNT_1(outputs & (MASK_OF(d) << (o*d)));
    max = MAX(max, val);
  }
  return max;
}

void row_first(wire_t numTotOuts, bitArray_t ret){
  bitArray_zero(numTotOuts, ret);
}

static bool row_tryNextOut__i(uint64_t *outputs, wire_t o, wire_t d, wire_t t){
  uint64_t outMask = MASK_OF(d) << (o*d);
  uint64_t upperTOutMask = MASK_OF(t) << (o*d + d-t);

  if(COUNT_1(*outputs & upperTOutMask) == t){ // all 1s are at the top -> no output conmbinations left
    *outputs &= ~outMask; // reset this out
    return 0;
  }

  wire_t fromBit;
  if(COUNT_1(*outputs & outMask) < t){
    fromBit = o*d; // can't overwrite the higer bits as t < d
  }else{ // COUNT_1(out) == t
    fromBit = TAIL_1(*outputs & outMask); // can't overwrite the higer bits due to the check with upperTOutMask
  }
  *outputs += 1ll << fromBit; // can't overwrite the higer bits, see above
  return 1;
}

bool row_tryNext_outLeqT(bitArray_t curr, wire_t numOuts, wire_t d, wire_t t){
  uint64_t outputs = bitArray_lowest_get(numOuts*d, curr, numOuts*d);
  for(wire_t o = 0; o < numOuts; o++){
    if(row_tryNextOut__i(&outputs, o, d, t)){
      bitArray_lowest_set(numOuts*d, curr, numOuts*d, outputs);
      return 1;
    }
    // the combination of output o has already been resetted
  }
  bitArray_lowest_set(numOuts*d, curr, numOuts*d, outputs);
  return 0;
}

bool row_tryNext_probeLeqMaxCoeff(bitArray_t curr, wire_t numMaskedOuts, wire_t numTotOuts, wire_t maxCoeff){
  if(bitArray_lowest_get(numTotOuts, curr, numMaskedOuts) != 0) FAIL("row.c: BUG: row_tryNextProbe's parameter includes an output bit\n");

  if(bitArray_count1(numTotOuts, curr) < (unsigned) maxCoeff)
    return !bitArray_localInc(numTotOuts, curr, numMaskedOuts);

  if(maxCoeff == 0) return 0; // only one probe combination: all zeros

  return !bitArray_localInc(numTotOuts, curr, bitArray_tail1(numTotOuts, curr));
}


// used as an iterator to get the next sub probe combination.
bool row_tryNext_subrow(wire_t numTotOuts, bitArray_t highestRow, bitArray_t curr){ // first is 0, return 0 for end
  if(bitArray_eq(numTotOuts, curr, highestRow)) return 0;

  BITARRAY_DEF_VAR(numTotOuts, curr_zeros)
  bitArray_xor(numTotOuts, highestRow, curr, curr_zeros); // curr_zeros has a 1 where curr has a meaningful 0
  wire_t lowest0 = bitArray_tail1(numTotOuts, curr_zeros); // incementing of 1 = setting the lowest 0 to 1 and all the lower 1s to 0.

  bitArray_set(numTotOuts, curr, lowest0);
  bitArray_lowest_reset(numTotOuts, curr, lowest0);
  return 1;
}

