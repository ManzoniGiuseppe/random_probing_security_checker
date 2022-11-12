//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _TRANSFORM_GENERATOR_H_
#define _TRANSFORM_GENERATOR_H_

#include "rowHashed.h"
#include "gadget.h"

typedef struct { void *transformGenerator; } transformGenerator_t;

T__THREAD_SAFE transformGenerator_t transformGenerator_alloc(rowHashed_t rows, gadget_t *g);
void transformGenerator_free(transformGenerator_t storage);

T__THREAD_SAFE void transformGenerator_getTranform(transformGenerator_t storage, rowHash_t row, wire_t numMaskedIns, wire_t numRnds, fixed_cell_t *ret_transform); // transform[1ll << numMaskedIns]

#endif // _TRANSFORM_GENERATOR_H_
