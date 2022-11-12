//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _HASH_CACHE_H_
#define _HASH_CACHE_H_

#include "types.h"
#include <stddef.h>

typedef struct { void *hashCache; } hashCache_t;

// hash_t with the current used bit = 0 is illegal. this means an all 0s index is always illegal.

T__THREAD_SAFE hashCache_t hashCache_new(size_t size_k, size_t size_v, T__ACQUIRES const char *of_what);
void hashCache_delete(hashCache_t ht);


T__THREAD_SAFE void hashCache_set(hashCache_t ht, void *key, void *value);
T__THREAD_SAFE bool hashCache_getValue(hashCache_t s, void *key, void *ret_value); // returns if present. If so, it sets ret_value



#endif // _HASH_CACHE_H_
