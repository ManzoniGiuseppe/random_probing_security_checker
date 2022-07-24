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
row_t row_first(void);
bool row_tryNextOut(row_t *curr);
bool row_tryNextProbeAndOut(row_t *curr);
bool row_tryGetNext(row_t highestRow, row_t *curr);


#define ITERATE_OVER_DIRECT_SUB_ROWS(p_row, sub, code) { \
  row_t hfuek__row = (p_row);                            \
  for(int hfuek__j = 0; hfuek__j < ROW_VALUES_SIZE; hfuek__j++){ \
    row_value_t hfuek__row_it = hfuek__row.values[hfuek__j];     \
    if(hfuek__row_it == 0) continue;    \
                                        \
    for(shift_t hfuek__i = TAIL_1(hfuek__row_it);  hfuek__i <= LEAD_1(hfuek__row_it);  hfuek__i++){ \
      row_t sub;                        \
      {                                 \
        row_value_t sub_it = hfuek__row_it &~ (1ll << hfuek__i);         \
        if(sub_it == hfuek__row_it) continue; /* it's not a sub-probe */ \
        sub = hfuek__row;               \
        sub.values[hfuek__j] = sub_it;  \
      }        \
               \
      { code } \
    }          \
  }            \
}




#endif // _ROW_H_

