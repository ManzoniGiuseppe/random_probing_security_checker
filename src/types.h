//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <assert.h>
#include <stddef.h>

//#define MAX_NUM_TOT_INS


// if it's safe to call a function in different thread, even at the same time with the same parameters (excluding return parameters) as any other functions with this same tag.
#define T__THREAD_SAFE
// if references to a parameter are kept even after its function ended.
#define T__ACQUIRES
// if pointer to internal variables are passed outside, either with a return or set to a parameter
#define T__LEAKS



typedef uint8_t bool;
typedef int_fast32_t wire_t;  // to indicate the number of a wire, input, or output, ... or the number of wires, inputs, outputs, ...
typedef int_fast32_t shift_t;  // to indicate something stored as a log_2, for various reasons
typedef size_t pow2size_t; //  a size_t guaranteed to be a power of 2


// fixed point notation 1.numTotIns
#if 1+MAX_NUM_TOT_INS <= 32-1
  typedef int32_t fixed_cell_t;
#elif 1+MAX_NUM_TOT_INS <= 64-1
  typedef int64_t fixed_cell_t;
#elif 1+MAX_NUM_TOT_INS <= 128-1
  typedef __int128 fixed_cell_t;
#else
  #error "unsupported 1+MAX_NUM_TOT_INS > 128-1"
#endif



T__THREAD_SAFE void types_print_err(const char* format, ...);
T__THREAD_SAFE void types_print_out(const char* format, ...);
T__THREAD_SAFE __attribute__((noreturn)) void types_exit(void);
double binomial(int n, int k);


// useful def
#define LEAD_1(x) (assert(x), 63 - __builtin_clzll((x)))
#define TAIL_1(x) LEAD_1((x)&-(x))
#define COUNT_1(x) __builtin_popcountll((x))
#define ROT(x, pos) ((((uint64_t)(x)) << (pos)) | (((uint64_t)(x)) >> (-(pos) & 63)))
#define MASK_OF(bits)   ( (1llu<<(bits)) - 1 )
#define ABS(x)    ((x) >= 0 ? (x) : -(x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define SWAP(TYPE, V1, V2) {\
  TYPE TYPES_SWAP__ifsfhfe = (V1);\
  (V1) = (V2);\
  (V2) = TYPES_SWAP__ifsfhfe;\
}

#define FAIL(...)  { types_print_err("-FAIL- " __VA_ARGS__); types_exit(); }
#define printf(...)  types_print_out(__VA_ARGS__)

#define ON_DBG(lvl, CODE) { if(DBG_LVL >= (lvl)) { CODE } }
#define DBG(lvl, ...) ON_DBG(lvl, { types_print_err("[" DBG_FILE "] " __VA_ARGS__); })

// production
#define DBG_LVL_NONE       0
// check and show bugs
#define DBG_LVL_TOFIX      1
// not insterested in that module
#define DBG_LVL_MINIMAL    2
// interested
#define DBG_LVL_DETAILED   3
// it's going badly
#define DBG_LVL_MAX        4


#define inline __attribute__((always_inline)) inline

#endif // _TYPES_H_
