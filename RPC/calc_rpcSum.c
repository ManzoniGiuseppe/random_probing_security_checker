#include <stdio.h>
#include <stdlib.h>

#include "calc.h"
#include "row.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


typedef uint64_t fixed_sum_t; // fixed point notation 2.(NUM_TOT_INS+1)
#define MAX_FIXED_SUM  (1ll << (NUM_TOT_INS+1))
#define UNINIT_SUM (MAX_FIXED_SUM+1)
#define FIXED_SUM_NORMALIZE(x)  ((x) > MAX_FIXED_SUM ? MAX_FIXED_SUM : (x))
// NOTE: assigning from fixed_cell_t to fixed_sum_t implies a /2


static inline fixed_sum_t* data_get(fixed_sum_t *data, hash_t row, col_t ii, shift_t bits){
  return & data[row + ii * (1ll << bits)];
}

static fixed_sum_t rowData_sumAbs(fixed_sum_t *rowData, row_t row, col_t ii){
  fixed_sum_t *it = data_get(rowData, rowTransform_transform_hash(row), ii, ROWTRANSFORM_RESERVATION_BITS);
  if(*it != UNINIT_SUM){
    return *it; // inited
  }

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  *it = 0;
  for(col_t i = 0; i < NUM_NORND_COLS  && *it < MAX_FIXED_SUM; i++){
    if((i &~ ii) != 0){
      *it += llabs(transform[i]);
    }
  }
  *it = FIXED_SUM_NORMALIZE(*it);
  return *it;
}

// second sum (without multeplicity)
static fixed_sum_t probeData_sumAbs(fixed_sum_t *probeData, fixed_sum_t *rowData, row_t row, col_t ii){
  fixed_sum_t *it = data_get(probeData, rowTransform_row_hash(row), ii, ROWTRANSFORM_ASSOC_BITS);
  if(*it != UNINIT_SUM){
    return *it; // inited
  }

  fixed_sum_t onlyRow = rowData_sumAbs(rowData, row, ii);
  if(onlyRow == MAX_FIXED_SUM){
    *it = MAX_FIXED_SUM;
    return MAX_FIXED_SUM;
  }

  /* check if any of the direct sub-probes has reached the max, if so then this is too */
  for(int j = 0; j < ROW_VALUES_SIZE; j++){
    row_value_t row_it = row.values[j];
    if(row_it == 0) continue;

    for(shift_t i = TAIL_1(row_it); i <= LEAD_1(row_it); i++){
      row_value_t sub_it = row_it &~ (1ll << i);
      if(sub_it == row_it) continue; // it's not a sub-probe
      row_t sub = row;
      sub.values[j] = sub_it;

      if(probeData_sumAbs(probeData, rowData, sub, ii) + onlyRow >= MAX_FIXED_SUM){
        *it = MAX_FIXED_SUM;
        return MAX_FIXED_SUM;
      }
    }
  }

  /* calculate them by summing the rows */
  *it = 0;
  row_t sub = row_first();
  do{
    *it += rowData_sumAbs(rowData, sub, ii);
  }while(*it < MAX_FIXED_SUM && row_tryGetNext(row, & sub));
  *it = FIXED_SUM_NORMALIZE(*it);
  return *it;
}

static shift_t maxShares_in(col_t value){
  shift_t ret = 0;
  col_t mask = (1ll << D)-1;
  for(int i = 0; i < NUM_INS; i++){
    int v = __builtin_popcountll(value & (mask << (i*D)));
    ret = v > ret ? v : ret;
  }
  return ret;
}

static fixed_sum_t probeData_min(fixed_sum_t *probeData, fixed_sum_t *rowData, probeComb_t probes, row_t output){
  row_t row = probeComb_getHighestRow(probes, output);

  fixed_sum_t ret = MAX_FIXED_SUM;
  for(col_t ii = 0; ii < NUM_NORND_COLS && ret != 0; ii++){
    if(maxShares_in(ii) <= T){
      fixed_sum_t val = probeData_sumAbs(probeData, rowData, row, ii);
      ret = MIN(ret, val);
    }
  }

  if(ret == 0){
    return 0; // to avoid bug in next condition if  MAX_FIXED_SUM >> shift_by == 0
  }

  shift_t shift_by = probeComb_getRowMulteplicity(probes);
  if(ret >= (MAX_FIXED_SUM >> shift_by) ) return MAX_FIXED_SUM; // nearly the same as asking if  (ret <<>

  return ret << shift_by;
}

coeff_t calc_rpcSum(void){
  // to store if the wanted row as any != 0 in the appropriate columns.
  fixed_sum_t* rowData = mem_calloc(sizeof(fixed_sum_t), (1ll << ROWTRANSFORM_RESERVATION_BITS) * NUM_NORND_COLS, "rowData for calc_rpcSum");
  for(hash_t i = 0; i < 1ll << ROWTRANSFORM_RESERVATION_BITS; i++)
    for(col_t ii = 0; ii < NUM_NORND_COLS; ii++)
      if(maxShares_in(ii) <= T)
        *data_get(rowData, i, ii, ROWTRANSFORM_RESERVATION_BITS) = UNINIT_SUM;

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  fixed_sum_t* probeData = mem_calloc(sizeof(fixed_sum_t), (1ll << ROWTRANSFORM_ASSOC_BITS) * NUM_NORND_COLS, "probeData for calc_rpcSum");
  for(hash_t i = 0; i < 1ll << ROWTRANSFORM_ASSOC_BITS; i++)
    for(col_t ii = 0; ii < NUM_NORND_COLS; ii++)
      if(maxShares_in(ii) <= T)
        *data_get(probeData, i, ii, ROWTRANSFORM_ASSOC_BITS) = UNINIT_SUM;

  coeff_t ret = coeff_zero();
  row_t output = row_first();
  do{
    if(row_maxShares(output) != T) continue; // the max will be in the ones with most outputs
    coeff_t curr = coeff_zero();

    ITERATE_OVER_PROBES(probes, coeffNum, {
      fixed_sum_t min = probeData_min(probeData, rowData, probes, output);
      if(min != 0){
        double mul = probeComb_getProbesMulteplicity(probes);
        curr.values[coeffNum] += min / (double) MAX_FIXED_SUM * mul;
      }
    })

    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));

  mem_free(rowData);
  mem_free(probeData);

  return ret;
}

