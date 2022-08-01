#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"

static inline double* data_get(double *data, hash_s_t row, col_t x, hash_s_t size){
  return & data[row + size * x];
}

static double *rowData;
static double *probeData_sum;
static double *probeData_min;
static size_t row_size;
static size_t probe_size;

static double xor_col(col_t v1, col_t v2){
  col_t v = v1 & v2;
  shift_t ones = __builtin_popcountll(v);
  return (ones % 2 == 0) ? 1 : -1;
}

static double rowData_sumPhase(row_t row, col_t x){
  double *it = data_get(rowData, rowTransform_transform_hash(row), x, row_size);
  if(!isnan(*it)){
    return *it; // inited
  }

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  *it = 0.0;
  for(col_t i = 1; i < 1ll<<NUM_INS; i++){ // 0 excluded
    *it += transform[calcUtils_intExpandByD(i)] / (double) (1ll << NUM_TOT_INS) * xor_col(i, x);
  }
  return *it;
}

static double xor_row(row_t v1, row_t v2){
  row_t v = row_and(v1, v2);
  int ones = row_numOnes(v);
  return (ones % 2 == 0) ? 1 : -1;
}

// without multeplicity
static double probeData_sumPhase(row_t row, row_t o, col_t x){
  double *it = data_get(probeData_sum, rowTransform_row_hash(row), x, probe_size);
  if(!isnan(*it)){
    return *it; // inited
  }

  *it = 0.0;
  row_t omega = row_first();
  do{
     *it += rowData_sumPhase(omega, x) * xor_row(o, omega);
  }while(row_tryGetNext(row, & omega));
  return *it;
}

static double probeData_evalMin(row_t row, col_t x){
  double *it = data_get(probeData_min, rowTransform_row_hash(row), x, probe_size);
  if(!isnan(*it)){
    return *it; // inited
  }

  *it = 0.0;
  row_t o = row_first();
  do{
    double val = probeData_sumPhase(row, o, x);
    *it += ABS(val);
  }while(row_tryGetNext(row, & o));

  *it = *it / 2 * ldexp(1.0, - row_numOnes(row)); // 2 ** - row_numOnes(row) = multeplicity * 2** - numprobes
  *it = MIN(1, *it);

  return *it;
}

// with multeplicity
static coeff_t toBeSummed(row_t highest_row, col_t x){
  double min = probeData_evalMin(highest_row, x);

  if(min == 0.0) return coeff_zero();

  return coeff_times(calcUtils_totProbeMulteplicity(highest_row), min);
}

static coeff_t sumProbes(col_t x){
  coeff_t ret = coeff_zero();
  row_t probes = row_first();
  do{
    ret = coeff_add(ret, toBeSummed(probes, x));
  }while(row_tryNextProbe(& probes));
  return ret;
}

coeff_t calc_rpsTeo(void){
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(double), row_size * (1ll << NUM_INS),  "rowData for calc_rpcTeo");
  for(hash_s_t row = 0; row < row_size; row++)
    for(col_t x = 0; x < (1ll << NUM_INS); x++)
      *data_get(rowData, row, x, row_size) = NAN;

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_sum = mem_calloc(sizeof(double), probe_size * (1ll << NUM_INS), "probeData_sum for calc_rpcTeo");
  for(hash_s_t row = 0; row < probe_size; row++)
    for(col_t x = 0; x < (1ll << NUM_INS); x++)
      *data_get(probeData_sum, row, x, probe_size) = NAN;

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_min = mem_calloc(sizeof(double), probe_size * (1ll << NUM_INS), "probeData_min for calc_rpcTeo");
  for(hash_s_t row = 0; row < probe_size; row++)
    for(col_t x = 0; x < (1ll << NUM_INS); x++)
      *data_get(probeData_min, row, x, probe_size) = NAN;



  coeff_t max = coeff_zero();
  for(col_t x = 0; x < 1ll<<NUM_INS; x++){
    max = coeff_max(max, sumProbes(x));
  }
  return max;
}
