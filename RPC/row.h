#ifndef _ROW_H_
#define _ROW_H_


#include "types.h"


#define ROW_VALUES_SIZE ((NUM_TOT_OUTS+63)/64)

typedef uint64_t row_value_t;
typedef struct {
  row_value_t values[ROW_VALUES_SIZE];
} row_t;


shift_t row_maxShares(row_t r);
row_t row_singleInput(shift_t input);
bool row_eq(row_t r0, row_t r1);
row_t row_or(row_t r0, row_t r1);
row_t row_and(row_t r0, row_t r1);
row_t row_xor(row_t r0, row_t r1);
row_t row_not(row_t r);
row_t row_first();
bool row_tryNextOut(row_t *curr);
bool row_tryNextProbeAndOut(row_t *curr);
bool row_tryGetNext(row_t highestRow, row_t *curr);


#endif // _ROW_H_

