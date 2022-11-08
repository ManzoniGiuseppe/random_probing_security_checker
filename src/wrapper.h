//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _WRAPPER_H_
#define _WRAPPER_H_

#include "types.h"
#include "row.h"
#include "subrowHashed.h"
#include "rowInfo.h"
#include "gadget.h"

typedef struct { void * wrapper; } wrapper_t;

T__THREAD_SAFE wrapper_t wrapper_new(
  gadget_t *g,
  wire_t maxCoeff,
  wire_t t,  // >= 0 for RPC, -1 for RPS
  rowInfo_generator_t gen,
  T__ACQUIRES const char *what
);

T__THREAD_SAFE pow2size_t wrapper_subrowSize(wrapper_t storage);
T__THREAD_SAFE bool wrapper_containsSubrow(wrapper_t storage, subrowHash_t subrow);
T__THREAD_SAFE T__LEAKS bitArray_t wrapper_getSubrow2Row(wrapper_t storage, subrowHash_t subrow);
#define WRAPPER_ITERATE_SUBROWS(wrapper, IT_SUBROW, IT_ROW, CODE) {\
  size_t wrapper_kvgdlgr_size = wrapper_subrowSize(wrapper);\
  subrowHash_t IT_SUBROW;\
  for(IT_SUBROW.v = 0; IT_SUBROW.v < wrapper_kvgdlgr_size; IT_SUBROW.v++){\
    if(wrapper_containsSubrow(wrapper, IT_SUBROW)){\
      bitArray_t IT_ROW = wrapper_getSubrow2Row(wrapper, IT_SUBROW);\
      { CODE }\
    }\
  }\
}

T__THREAD_SAFE T__LEAKS void *wrapper_getRowInfo(wrapper_t storage, bitArray_t sub);
void wrapper_freeRowInfo(wrapper_t storage);
T__THREAD_SAFE void wrapper_startCalculatingCoefficients(wrapper_t storage);
T__THREAD_SAFE subrowHash_t wrapper_getRow2Subrow(wrapper_t storage, bitArray_t row);
void wrapper_delete(wrapper_t storage);




#endif // _WRAPPER_H_
