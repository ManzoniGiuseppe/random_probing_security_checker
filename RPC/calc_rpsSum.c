#include <stdlib.h>

#include "calc.h"
#include "row.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


#if NUM_TOT_INS+1+2 <= 16
  typedef uint16_t fixed_sum_t; // fixed point notation 2.(NUM_TOT_INS+1)
#elif NUM_TOT_INS+1+2 <= 32
  typedef uint32_t fixed_sum_t; // fixed point notation 2.(NUM_TOT_INS+1)
#else
  typedef uint64_t fixed_sum_t; // fixed point notation 2.(NUM_TOT_INS+1)
#endif

#define MAX_FIXED_SUM  (1ll << (NUM_TOT_INS+1))
#define UNINIT_SUM (MAX_FIXED_SUM+1)
#define FIXED_SUM_NORMALIZE(x)  ((x) > MAX_FIXED_SUM ? MAX_FIXED_SUM : (x))
// NOTE: assigning from fixed_cell_t to fixed_sum_t implies a /2


// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
static col_t intExpandByD(col_t val){
  if(val == 0) return 0;
  col_t bit = (1ll<<D)-1;
  col_t ret = 0;
  for(shift_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << (i*D);
  }
  return ret;
}

static fixed_sum_t rowData_sumAbs(fixed_sum_t *rowData, row_t row){
  fixed_sum_t *it = & rowData[rowTransform_transform_hash(row)];
  if(*it != UNINIT_SUM) return *it; // inited

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  *it = 0;
  for(int i = 1; i < (1ll<<NUM_INS) && *it < MAX_FIXED_SUM; i++){
    *it += llabs(transform[intExpandByD(i)]);
  }
  *it = FIXED_SUM_NORMALIZE(*it);
  return *it;
}

// second sum (without multeplicity)
static fixed_sum_t probeData_sumAbs(fixed_sum_t *probeData, fixed_sum_t *rowData, row_t row){
  fixed_sum_t *it = & probeData[rowTransform_row_hash(row)];
  if(*it != UNINIT_SUM) return *it; // inited

  fixed_sum_t onlyRow = rowData_sumAbs(rowData, row);
  if(onlyRow == MAX_FIXED_SUM){
    *it = MAX_FIXED_SUM;
    return MAX_FIXED_SUM;
  }

  /* check if any of the direct sub-probes has reached the max, if so then this is too */
  ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
    if(probeData_sumAbs(probeData, rowData, sub) + onlyRow >= MAX_FIXED_SUM){
      *it = MAX_FIXED_SUM;
      return MAX_FIXED_SUM;
    }
  })

  /* calculate them by summing the rows */
  *it = 0;
  row_t sub = row_first();
  do{
    *it += rowData_sumAbs(rowData, sub);
  }while(*it < MAX_FIXED_SUM && row_tryGetNext(row, & sub));
  *it = FIXED_SUM_NORMALIZE(*it);
  return *it;
}


// factor in the multeplicity, return the min(1, 1/2 * ...)
static fixed_sum_t probeData_min(fixed_sum_t *probeData, fixed_sum_t *rowData, probeComb_t probes){
  shift_t shift_by = probeComb_getRowMulteplicity(probes);
  fixed_sum_t ret = probeData_sumAbs(probeData, rowData, probeComb_getHighestRow_noOut(probes));

  if(ret == 0) return 0; // to avoid bug in the next line if  MAX_FIXED_SUM >> shift_by == 0
  if(ret >= (MAX_FIXED_SUM >> shift_by) ) return MAX_FIXED_SUM; // nearly the same as asking if  (ret << shift_by) >= MAX_FIXED_SUM

  return ret << shift_by;
}



coeff_t calc_rpsSum(void){
  // to store if the wanted row as any != 0 in the appropriate columns.
  fixed_sum_t* rowData = mem_calloc(sizeof(fixed_sum_t), 1ll << ROWTRANSFORM_TRANSFORM_BITS, "rowData for calc_rpsSum");
  for(hash_s_t i = 0; i < 1ll << ROWTRANSFORM_TRANSFORM_BITS; i++)
    rowData[i] = UNINIT_SUM;

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  fixed_sum_t* probeData = mem_calloc(sizeof(fixed_sum_t), 1ll << ROWTRANSFORM_ROW_BITS, "probeData for calc_rpsSum");
  for(hash_s_t i = 0; i < 1ll << ROWTRANSFORM_ROW_BITS; i++)
    probeData[i] = UNINIT_SUM;

  coeff_t ret = coeff_zero();

  ITERATE_OVER_PROBES(probes, coeffNum, {
    double mul = probeComb_getProbesMulteplicity(probes);
    fixed_sum_t sum = probeData_min(probeData, rowData, probes);
    ret.values[coeffNum] += sum / (double) MAX_FIXED_SUM * mul;
  })


  mem_free(rowData);
  mem_free(probeData);

  return ret;
}
