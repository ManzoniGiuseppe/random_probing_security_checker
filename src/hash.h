//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _HASH_H_
#define _HASH_H_

#include "types.h"


#define HASH_WIDTH  32
typedef struct { uint_fast32_t v; } hash_t;
typedef struct { uint_least32_t v; } hash_compact_t;

T__THREAD_SAFE hash_t hash_calculate(size_t size, void *value, unsigned int counter);

#define hash_eq(h1, h2)  ((h1).v == (h2).v)
#define hash_toCompact(h)  ((hash_compact_t){ (h).v })
#define hash_fromCompact(h)  ((hash_t){ (h).v })

#endif // _HASH_H_
