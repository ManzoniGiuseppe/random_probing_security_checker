//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _MEM_H_
#define _MEM_H_

#include <stddef.h>
#include "types.h"


T__THREAD_SAFE void* mem_calloc(size_t size, uint64_t elems, T__ACQUIRES const char *what);
T__THREAD_SAFE void mem_free(void* mem);

#endif // _MEM_H_

