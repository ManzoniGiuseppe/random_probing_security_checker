#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


// fixed point notation (1+D*NUM_INS).NUM_TOT_INS
#if NUM_TOT_INS+1 + D*NUM_INS <= 16-1
  typedef int16_t fixed_rowsum_t;
#elif NUM_TOT_INS+1 + D*NUM_INS <= 32-1
  typedef int32_t fixed_rowsum_t;
#elif NUM_TOT_INS+1 + D*NUM_INS <= 64-1
  typedef int64_t fixed_rowsum_t;
#else
  typedef double fixed_rowsum_t;
#endif

// fixed point notation (1+D*NUM_INS + T*NUM_OUTS + MAX_COEFF).NUM_TOT_INS
#if NUM_TOT_INS+1 + D*NUM_INS + T*NUM_OUTS + MAX_COEFF <= 16-1
  typedef int16_t fixed_probesum_t;
#elif NUM_TOT_INS+1 + D*NUM_INS + T*NUM_OUTS + MAX_COEFF <= 32-1
  typedef int32_t fixed_probesum_t;
#elif NUM_TOT_INS+1 + D*NUM_INS + T*NUM_OUTS + MAX_COEFF <= 64-1
  typedef int64_t fixed_probesum_t;
#else
  typedef double fixed_probesum_t;
#endif


static fixed_rowsum_t *rowData;
static double *probeData_min;
static size_t row_size;
static size_t probe_size;



static inline fixed_rowsum_t* rowData_get(row_t row, int ii_index, col_t x){
  return & rowData[x + NUM_NORND_COLS * (rowTransform_row_hash(row) + row_size * ii_index)];
}

static inline double* probeData_min_get(row_t row){
  return & probeData_min[rowTransform_subRow_hash(row)];
}

static int xor_col(col_t v1, col_t v2){
  col_t v = v1 & v2;
  shift_t ones = __builtin_popcountll(v);
  return (ones % 2 == 0) ? 1 : -1;
}

static void rowData_init(row_t row, col_t ii, int ii_index, col_t x){
  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  fixed_rowsum_t *it = rowData_get(row, ii_index, x);

  *it = 0.0;
  for(col_t i = 0; i < NUM_NORND_COLS; i++){
    if((i &~ ii) != 0){
      *it += transform[i] * xor_col(i, x);
    }
  }
}

static inline fixed_rowsum_t rowData_sumPhase(row_t row, int ii_index, col_t x){
  return *rowData_get(row, ii_index, x);
}

static int xor_row(row_t v1, row_t v2){
  row_t v = row_and(v1, v2);
  int ones = row_numOnes(v);
  return (ones % 2 == 0) ? 1 : -1;
}

// without multeplicity
static double probeData_sumPhase(row_t row, row_t o, int ii_index, col_t x){
  fixed_probesum_t it = 0.0;
  row_t k = row_first();
  do{
     it += rowData_sumPhase(k, ii_index, x) * xor_row(o, k);
  }while(row_tryGetNext(row, & k));

  return it / (double) (1ll << NUM_TOT_INS);
}

static double probeData_sumAfterAbs(row_t row, int ii_index, col_t x){
  double ret = 0.0;
  row_t o = row_first();
  do{
    double val = probeData_sumPhase(row, o, ii_index, x);
    ret += ABS(val);
  }while(row_tryGetNext(row, & o));
  return ret;
}

static double probeData_minMax(row_t row){
  double min = DBL_MAX;
  ITERATE_II({
    double max = 0.0;
    ITERATE_X_ACT({
      double val = probeData_sumAfterAbs(row, ii_index, x);
      max = MAX(val, max);
    })
    min = MIN(min, max);
  })
  return min;
}

static double probeData_min1(row_t row){
  double ret = probeData_minMax(row);

  ret = ret / 2 * ldexp(1.0, - row_numOnes(row)); // 2 ** - row_numOnes(row) = multeplicity * 2** - numprobes
  ret = MIN(1, ret);
  return ret;
}

static void probeData_init(row_t row){
  *probeData_min_get(row) = probeData_min1(row);
}

static coeff_t sumOfProbes(row_t out){
  coeff_t sum = coeff_zero();
  row_t probes = row_first();
  do{
    row_t row = row_or(probes, out);
    sum = coeff_add(sum, coeff_times(calcUtils_totProbeMulteplicity(row), *probeData_min_get(row)));
  }while(row_tryNextProbe(& probes));
  return sum;
}


coeff_t calc_rpcTeo(void){
  printf("rpcTeo: 0/3\n");
  row_size = rowTransform_row_hash_size();
  probe_size = rowTransform_subRow_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(fixed_rowsum_t), row_size * II_USED_COMB * NUM_NORND_COLS,  "rowData for calc_rpcTeo");
  ITERATE_PROBE_AND_OUT(IPT_ROW, {
    ITERATE_II({
      ITERATE_X_ACT({
        rowData_init(probeAndOut, ii, ii_index, x);
      })
    })
  })
  printf("rpcTeo: 1/3\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_min = mem_calloc(sizeof(double), probe_size, "probeData_min for calc_rpcTeo");
  ITERATE_PROBE_AND_OUT(IPT_SUBROW, {
    probeData_init(probeAndOut);
  })
  mem_free(rowData);
  printf("rpcTeo: 2/3\n");

  coeff_t ret = coeff_zero();
  row_t output = row_first();
  do{
    coeff_t curr = sumOfProbes(output);
    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));

  mem_free(probeData_min);
  printf("rpcTeo: 3/3\n");
  return ret;
}
