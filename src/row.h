//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _ROW_H_
#define _ROW_H_

#include "types.h"
#include "bitArray.h"



wire_t row_maxShares(bitArray_t r, wire_t numOuts, wire_t d);
void row_first(wire_t numTotOuts, bitArray_t ret);
bool row_tryNext_outLeqT(bitArray_t curr, wire_t numOuts, wire_t d, wire_t t); // automatically resets the output combination
bool row_tryNext_probeLeqMaxCoeff(bitArray_t curr, wire_t numMaskedOuts, wire_t numTotOuts, wire_t maxCoeff); // needs the outputs to be 0
bool row_tryNext_subrow(wire_t numTotOuts, bitArray_t highestRow, bitArray_t curr);


#define ROW_ITERATE_OVER_ONES(numTotOuts, row, I, CODE) {\
  BITARRAY_DEF_VAR(numTotOuts, row_hfuek__row)\
  bitArray_copy(numTotOuts, row, row_hfuek__row);\
  while(bitArray_count1(numTotOuts, row_hfuek__row) != 0){\
    wire_t I = bitArray_tail1(numTotOuts, row_hfuek__row);\
    bitArray_reset(numTotOuts, row_hfuek__row, I);\
    { CODE }\
  }\
}

#define ITERATE_OVER_DIRECT_SUB_ROWS(numTotOuts, row, SUB, CODE) {\
  BITARRAY_DEF_VAR(numTotOuts, SUB)\
  bitArray_copy(numTotOuts, row, SUB);\
  ROW_ITERATE_OVER_ONES(numTotOuts, SUB, row_hfuek__i, {\
    bitArray_reset(numTotOuts, SUB, row_hfuek__i);\
    { CODE }\
    bitArray_set(numTotOuts, SUB, row_hfuek__i);\
  })\
}


#define ITERATE_OVER_SUB_ROWS(numTotOuts, row, SUB, CODE) {\
  BITARRAY_DEF_VAR(numTotOuts, SUB)\
  row_first(numTotOuts, SUB);\
  do{\
    { CODE }\
  }while(row_tryNext_subrow(numTotOuts, row, SUB));\
}


#endif // _ROW_H_
