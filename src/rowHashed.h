//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _ROW_HASHED_H_
#define _ROW_HASHED_H_

#include "row.h"
#include "hash.h"

typedef struct { void *rowHashed; } rowHashed_t;

#if HASH_WIDTH <= 32
  typedef struct { uint_fast32_t v; } rowHash_t;
#else
  #error "unsupported HASH_WIDTH > 32"
#endif


T__THREAD_SAFE rowHashed_t rowHashed_alloc(bitArray_iterator_t itRows, wire_t numTotOuts);
void rowHashed_free(rowHashed_t storage);

T__THREAD_SAFE pow2size_t rowHashed_getSize(rowHashed_t storage);
T__THREAD_SAFE wire_t rowHashed_numTotOuts(rowHashed_t storage);
T__THREAD_SAFE bool rowHashed_contains(rowHashed_t storage, bitArray_t row);
T__THREAD_SAFE rowHash_t rowHashed_hash(rowHashed_t storage, bitArray_t row); // assumes the row is present.
T__THREAD_SAFE T__LEAKS bitArray_t rowHashed_get(rowHashed_t storage, rowHash_t index);
T__THREAD_SAFE bool rowHashed_hasVal(rowHashed_t storage, rowHash_t ret);

#define ROWHASHED_ITERATE(rows, RH, CODE) {\
  pow2size_t ROWHASHED_ITERATE__size = rowHashed_getSize(rows);\
  rowHash_t RH;\
  for(RH.v = 0; RH.v < ROWHASHED_ITERATE__size; RH.v++){\
    if(rowHashed_hasVal(rows, RH)){\
      { CODE }\
    }\
  }\
}


#endif //  _ROW_HASHED_H_
