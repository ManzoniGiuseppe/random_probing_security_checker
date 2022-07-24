#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>


// TODO: ensure that sizes are always enough

typedef uint8_t bool;
typedef uint32_t col_t;
typedef uint64_t hash_l_t;
typedef uint32_t hash_s_t;
typedef int32_t shift_t;
typedef int32_t fixed_cell_t;  // fixed point notation 1.NUM_TOT_INS  // cell of the transform TODO: rename


// ensure evering fits in the chosen types
#if NUM_COLS > INT32_MAX
#error "too many columns"
#endif


// useful def
#define LEAD_1(x) (63 - __builtin_clzll((x)))
#define TAIL_1(x) LEAD_1((x)&-(x))

#define ABS(x)    ((x) >= 0 ? (x) : -(x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define FAIL(...)  { fprintf(stderr, __VA_ARGS__); exit(1); }



#endif // _TYPES_H_

