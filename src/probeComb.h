//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _PROBECOMB_H_
#define _PROBECOMB_H_


#include "types.h"
#include "gadget.h"
#include "row.h"

typedef int *probeComb_t;

T__THREAD_SAFE void probeComb_getHighestRow(probeComb_t curr_comb, gadget_t *g, bitArray_t ret);

// it returns the logaritm, how much it needs to be shifted
T__THREAD_SAFE shift_t probeComb_getRowMulteplicity(probeComb_t curr_comb, gadget_t *g);

bool probeComb_tryIncrement(probeComb_t curr_comb, wire_t* curr_count, wire_t maxCoeff, gadget_t *g);

#define ITERATE_OVER_PROBES(maxCoeff, g, iterator_comb, iterator_count, CODE)  { \
    int iterator_comb[(g)->numProbes];\
    memset(iterator_comb, 0, sizeof(int) * (g)->numProbes);\
    wire_t iterator_count = 0; \
    do{ \
      { CODE } \
    }while(probeComb_tryIncrement(iterator_comb, &iterator_count, maxCoeff, g)); \
  }


T__THREAD_SAFE double probeComb_getProbesMulteplicity(probeComb_t curr_comb, gadget_t *g);

T__THREAD_SAFE void probeComb_firstWhoseImageIs(bitArray_t highest_row, probeComb_t ret_comb, wire_t *ret_count, wire_t maxCoeff, gadget_t *g);
bool probeComb_nextWithSameImage(probeComb_t curr_comb, wire_t *curr_count, wire_t maxCoeff, gadget_t *g);

#define ITERATE_OVER_PROBES_WHOSE_IMAGE_IS(maxCoeff, g, highest_row, iterator_comb, iterator_count, code)  { \
    int iterator_comb[(g)->numProbes];\
    memset(iterator_comb, 0, sizeof(int) * (g)->numProbes);\
    wire_t iterator_count = 0; \
    probeComb_firstWhoseImageIs(highest_row, iterator_comb, &iterator_count, maxCoeff, g); \
    do{ \
      code \
    }while(probeComb_nextWithSameImage(iterator_comb, &iterator_count, maxCoeff, g)); \
  }


#endif // _PROBECOMB_H_

