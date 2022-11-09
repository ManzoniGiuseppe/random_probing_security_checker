//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _SUBROW_HASHED_H_
#define _SUBROW_HASHED_H_

#include "types.h"
#include "rowHashed.h"

typedef struct { void *subrowHashed; } subrowHashed_t;

#if HASH_WIDTH <= 32
  typedef struct { uint_fast32_t v; } subrowHash_t;
#else
  #error "unsupported HASH_WIDTH > 32"
#endif

T__THREAD_SAFE subrowHashed_t subrowHashed_new(rowHashed_t rows, bitArray_iterator_t itRows, void *infoR2I, hash_t (*row2info)(void* infoR2I, rowHash_t row));
void subrowHashed_delete(subrowHashed_t storage);


T__THREAD_SAFE bool subrowHashed_containsSubrow(subrowHashed_t storage, subrowHash_t subrow);
T__THREAD_SAFE subrowHash_t subrowHashed_row2subrow(subrowHashed_t storage, rowHash_t row);
T__THREAD_SAFE rowHash_t subrowHashed_subrow2row(subrowHashed_t storage, subrowHash_t subrow);
T__THREAD_SAFE pow2size_t subrowHashed_numUniqueBySubrow(subrowHashed_t storage);

#endif // _SUBROW_HASHED_H_
