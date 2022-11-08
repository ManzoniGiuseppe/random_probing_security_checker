//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _ROW_INFO_H_
#define _ROW_INFO_H_

#include "transformGenerator.h"
#include "rowHashed.h"
#include "rowIndexedSet.h"

typedef struct {
  bool hashTheTransforms;
  size_t infoSize;
  void *getInfo_param;
  T__THREAD_SAFE void (*getInfo)(void *getInfo_param, wire_t d, wire_t numIns, fixed_cell_t *transform, void *ret_info); // transform[d*numIns], ret is initialized to all 0s.
} rowInfo_generator_t;

T__THREAD_SAFE rowIndexedSet_t rowInfo_build(transformGenerator_t tg, rowHashed_t rows, rowInfo_generator_t gen, wire_t d, wire_t numIns, wire_t numRnds);


#endif // _ROW_INFO_H_
