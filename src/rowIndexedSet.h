//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _ROW_INDEXED_SET_H_
#define _ROW_INDEXED_SET_H_

#include "types.h"
#include "rowHashed.h"
#include "hashMap.h"

typedef struct{ void * rowIndexedSet; } rowIndexedSet_t;

T__THREAD_SAFE rowIndexedSet_t rowIndexedSet_new(pow2size_t numRowHashed, size_t size_v, T__ACQUIRES char *ofWhat);

void rowIndexedSet_delete(rowIndexedSet_t ht);

// key may or may not be present.
void rowIndexedSet_add(rowIndexedSet_t ht, rowHash_t row, void *val);

T__THREAD_SAFE pow2size_t rowIndexedSet_currSize(rowIndexedSet_t ht);
T__THREAD_SAFE bool rowIndexedSet_containsRow(rowIndexedSet_t ht, rowHash_t row);
// rowHashed must have been inserted
T__THREAD_SAFE hash_t rowIndexedSet_row2valIndex(rowIndexedSet_t ht, rowHash_t row);
// keyIndex must be valid
T__THREAD_SAFE T__LEAKS void* rowIndexedSet_getVal(rowIndexedSet_t ht, hash_t valIndex);

T__THREAD_SAFE bool rowIndexedSet_hasVal(rowIndexedSet_t ht, reducedHash_t valIndex, void **ret_key);


#define ROWINDEXEDSET_ITERATE(map, REDUCED_INDEX, VALINDEX, CODE) {\
  size_t ROWINDEXEDSET_ITERATE__size = rowIndexedSet_currSize(map);\
  for(reducedHash_t REDUCED_INDEX = 0; REDUCED_INDEX < ROWINDEXEDSET_ITERATE__size; REDUCED_INDEX++){\
    void *VALINDEX;\
    if(rowIndexedSet_hasVal(map, REDUCED_INDEX, &VALINDEX)){\
      { CODE }\
    }\
  }\
}



#endif //  _ROW_INDEXED_SET_H_
