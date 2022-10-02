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
#elif NUM_TOT_INS+1+2 <= 64
  typedef uint64_t fixed_sum_t; // fixed point notation 2.(NUM_TOT_INS+1)
#else
  #error "Not enough bits for fixed_sum_t"
#endif

#define MAX_FIXED_SUM  (1ll << (NUM_TOT_INS+1))
#define FIXED_SUM_NORMALIZE(x)  ((x) > MAX_FIXED_SUM ? MAX_FIXED_SUM : (x))
// NOTE: assigning from fixed_cell_t to fixed_sum_t implies a /2


static fixed_sum_t *rowData;
static fixed_sum_t *probeData;
static size_t row_size;
static size_t probe_size;

static void rowData_init(row_t row){
  fixed_sum_t *it = & rowData[rowTransform_row_hash(row)];

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  *it = 0;
  for(int i = 1; i < (1ll<<NUM_INS) && *it < MAX_FIXED_SUM; i++){
    *it += llabs(transform[calcUtils_intExpandByD(i)]);
  }
  *it = FIXED_SUM_NORMALIZE(*it);
}

static fixed_sum_t rowData_sumAbs(row_t row){
  return rowData[rowTransform_row_hash(row)];
}

// second sum (without multeplicity)
static fixed_sum_t probeData_sumAbs(row_t row);
static void probeData_init(row_t row){
  fixed_sum_t *it = & probeData[rowTransform_subRow_hash(row)];

  fixed_sum_t onlyRow = rowData_sumAbs(row);
  if(onlyRow == MAX_FIXED_SUM){
    *it = MAX_FIXED_SUM;
    return;
  }

  /* check if any of the direct sub-probes has reached the max, if so then this is too */
  ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
    if(probeData_sumAbs(sub) + onlyRow >= MAX_FIXED_SUM){
      *it = MAX_FIXED_SUM;
      return;
    }
  })

  /* calculate them by summing the rows */
  *it = 0;
  row_t sub = row_first();
  do{
    *it += rowData_sumAbs(sub);
  }while(*it < MAX_FIXED_SUM && row_tryGetNext(row, & sub));
  *it = FIXED_SUM_NORMALIZE(*it);
}

static fixed_sum_t probeData_sumAbs(row_t row){
  return probeData[rowTransform_subRow_hash(row)];
}


// factor in the multeplicity, return the min(1, 1/2 * ...)
static fixed_sum_t probeData_min(probeComb_t probes){
  shift_t shift_by = probeComb_getRowMulteplicity(probes);
  fixed_sum_t ret = probeData_sumAbs(probeComb_getHighestRow_noOut(probes));

  if(ret == 0) return 0; // to avoid bug in the next line if  MAX_FIXED_SUM >> shift_by == 0
  if(ret >= (MAX_FIXED_SUM >> shift_by) ) return MAX_FIXED_SUM; // nearly the same as asking if  (ret << shift_by) >= MAX_FIXED_SUM

  return ret << shift_by;
}



coeff_t calc_rpsSum(void){
  printf("rpsSum: 0/3\n");
  row_size = rowTransform_row_hash_size();
  probe_size = rowTransform_subRow_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(fixed_sum_t), row_size, "rowData for calc_rpsSum");
  ITERATE_PROBE(IPT_ROW, {
    rowData_init(probe);
  })
  printf("rpsSum: 1/3\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData = mem_calloc(sizeof(fixed_sum_t), probe_size, "probeData for calc_rpsSum");
  ITERATE_PROBE(IPT_SUBROW, {
    probeData_init(probe);
  })
  mem_free(rowData);
  printf("rpsSum: 2/3\n");

  coeff_t ret = coeff_zero();
  ITERATE_OVER_PROBES(probes, coeffNum, {
    double mul = probeComb_getProbesMulteplicity(probes);
    fixed_sum_t sum = probeData_min(probes);
    ret.values[coeffNum] += sum / (double) MAX_FIXED_SUM * mul;
  })

  mem_free(probeData);
  printf("rpsSum: 3/3\n");
  return ret;
}
