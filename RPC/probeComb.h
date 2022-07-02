#ifndef _PROBECOMB_H_
#define _PROBECOMB_H_


#include "types.h"
#include "row.h"

typedef int probeComb_t[NUM_PROBES];

row_t probeComb_getHighestRow(probeComb_t curr_comb, row_t output);
#define probeComb_getHighestRow_noOut(curr_comb) (probeComb_getHighestRow((curr_comb), row_first()))

// return the logaritm, how much it needs to be shifted
shift_t probeComb_getRowMulteplicity(probeComb_t curr_comb);

bool probeComb_tryIncrement(probeComb_t curr_comb, shift_t* curr_count);

#define ITERATE_OVER_PROBES(iterator_comb, iterator_count, code)  { \
    probeComb_t iterator_comb = {0}; \
    shift_t iterator_count = 0; \
    do{ \
      code \
    }while(probeComb_tryIncrement(iterator_comb, &iterator_count)); \
  }


double probeComb_getProbesMulteplicity(probeComb_t curr_comb);


#endif // _PROBECOMB_H_

