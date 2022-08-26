#include <stdio.h>
#include <stdlib.h>

#include "calc.h"
#include "calcUtils.h"
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
#define FIXED_SUM_NORMALIZE(x)  ((x) > MAX_FIXED_SUM ? MAX_FIXED_SUM : (x))
// NOTE: assigning from fixed_cell_t to fixed_sum_t implies a /2



static inline fixed_sum_t* data_get(fixed_sum_t *data, hash_s_t row, int ii_index, hash_s_t size){
  return & data[row + ii_index * size];
}

static fixed_sum_t *rowData;
static fixed_sum_t* probeData;
static size_t row_size;
static size_t probe_size;


static void rowData_init(row_t row, col_t ii, int ii_index){
  fixed_sum_t *it = data_get(rowData, rowTransform_transform_hash(row), ii_index, row_size);

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  *it = 0;
  for(col_t i = 0; i < NUM_NORND_COLS  && *it < MAX_FIXED_SUM; i++){
    if((i &~ ii) != 0){
      *it += llabs(transform[i]);
    }
  }
  *it = FIXED_SUM_NORMALIZE(*it);
}

static fixed_sum_t rowData_sumAbs(row_t row, int ii_index){
  return *data_get(rowData, rowTransform_transform_hash(row), ii_index, row_size);
}

// second sum (without multeplicity)
static fixed_sum_t probeData_sumAbs(row_t row, int ii_index);
static void probeData_init(row_t row, __attribute__((unused)) col_t ii, int ii_index){
  fixed_sum_t *it = data_get(probeData, rowTransform_row_hash(row), ii_index, probe_size);

  fixed_sum_t onlyRow = rowData_sumAbs(row, ii_index);
  if(onlyRow >= MAX_FIXED_SUM){
    *it = MAX_FIXED_SUM; // this can be done as curr_min depneds entirely on the row's content.
    return;
  }

  /* the value of this probe is higher than the one of its sub probes */
  ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
    fixed_sum_t sub_val = probeData_sumAbs(sub, ii_index);
    if(sub_val + onlyRow >= MAX_FIXED_SUM){
      *it = MAX_FIXED_SUM;
      return;
    }
  })

  /* calculate them by summing the rows */
  *it = 0;
  row_t sub = row_first();
  do{
    *it += rowData_sumAbs(sub, ii_index);
  }while(*it < MAX_FIXED_SUM && row_tryGetNext(row, & sub));
  *it = FIXED_SUM_NORMALIZE(*it);
}

static fixed_sum_t probeData_sumAbs(row_t row, int ii_index){
  return *data_get(probeData, rowTransform_row_hash(row), ii_index, probe_size);
}


static fixed_sum_t probeData_min(probeComb_t probes, row_t output){
  row_t row = probeComb_getHighestRow(probes, output);

  fixed_sum_t ret = MAX_FIXED_SUM;
  int ii_index = 0;
  for(col_t ii = 0; ii < NUM_NORND_COLS && ret != 0; ii++){
    if(calcUtils_maxSharesIn(ii) <= T){
      fixed_sum_t val = probeData_sumAbs(row, ii_index);
      ret = MIN(ret, val);
      ii_index++;
    }
  }

  if(ret == 0){
    return 0; // to avoid bug in next condition if  MAX_FIXED_SUM >> shift_by == 0
  }

  shift_t shift_by = probeComb_getRowMulteplicity(probes);
  if(ret >= (MAX_FIXED_SUM >> shift_by) ) return MAX_FIXED_SUM; // nearly the same as asking if  ret << shift_by >= MAX_FIXED_SUM

  return ret << shift_by;
}

coeff_t calc_rpcSum(void){
  printf("rpcSum: 0/3\n");
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(fixed_sum_t), row_size * II_USED_COMB, "rowData for calc_rpcSum");
  calcUtils_init_outIi(1, rowData_init);
  printf("rpcSum: 1/3\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData = mem_calloc(sizeof(fixed_sum_t), probe_size * II_USED_COMB, "probeData for calc_rpcSum");
  calcUtils_init_outIi(0, probeData_init);
  mem_free(rowData);
  printf("rpcSum: 2/3\n");

  coeff_t ret = coeff_zero();
  row_t output = row_first();
  do{
    if(row_maxShares(output) != T) continue; // the max will be in the ones with most outputs
    coeff_t curr = coeff_zero();

    ITERATE_OVER_PROBES(probes, coeffNum, {
      fixed_sum_t min = probeData_min(probes, output);
      if(min != 0){
        double mul = probeComb_getProbesMulteplicity(probes);
        curr.values[coeffNum] += min / (double) MAX_FIXED_SUM * mul;
      }
    })

    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));

  mem_free(probeData);
  printf("rpcSum: 3/3\n");
  return ret;
}

