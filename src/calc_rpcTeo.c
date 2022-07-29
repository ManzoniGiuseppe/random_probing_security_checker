#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


static inline double* data_get(double *data, hash_s_t row, col_t ii, col_t x, hash_s_t size){
  return & data[x + NUM_NORND_COLS * (row + size * ii)];
}

static double *rowData;
static double *probeData_sum;
static double *probeData_min;
static hash_s_t row_size;
static hash_s_t probe_size;

static double xor_col(col_t v1, col_t v2){
  col_t v = v1 & v2;
  shift_t ones = __builtin_popcountll(v);
  return (ones % 2 == 0) ? 1 : -1;
}

static double rowData_sumPhase(row_t row, col_t ii, col_t x){
  double *it = data_get(rowData, rowTransform_transform_hash(row), ii, x, row_size);
  if(!isnan(*it)){
    return *it; // inited
  }

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  *it = 0.0;
  for(col_t i = 0; i < NUM_NORND_COLS; i++){
    if((i &~ ii) != 0){
      *it += transform[i] / (double) (1ll << NUM_TOT_INS) * xor_col(i, x);
    }
  }
  return *it;
}

static double xor_row(row_t v1, row_t v2){
  row_t v = row_and(v1, v2);
  int ones = row_numOnes(v);
  return (ones % 2 == 0) ? 1 : -1;
}

// without multeplicity
static double probeData_sumPhase(row_t row, row_t o, col_t ii, col_t x){
  double *it = data_get(probeData_sum, rowTransform_row_hash(row), ii, x, probe_size);
  if(!isnan(*it)){
    return *it; // inited
  }

  *it = 0.0;
  row_t omega = row_first();
  do{
     *it += rowData_sumPhase(omega, ii, x) * xor_row(o, omega);
  }while(row_tryGetNext(row, & omega));
  return *it;
}

static double probeData_evalMin(row_t row, col_t ii, col_t x){
  double *it = data_get(probeData_min, rowTransform_row_hash(row), ii, x, probe_size);
  if(!isnan(*it)){
    return *it; // inited
  }

  *it = 0.0;
  row_t o = row_first();
  do{
    double val = probeData_sumPhase(row, o, ii, x);
    *it = ABS(val);
  }while(row_tryGetNext(row, & o));

  *it = *it / 2 * ldexp(1.0, - row_numOnes(row)); // 2 ** - row_numOnes(row) = multeplicity * 2** - numprobes
  *it = MIN(1, *it);

  return *it;
}

// with multeplicity
static coeff_t toBeSummed(row_t highest_row, col_t ii, col_t x){
  double min = probeData_evalMin(highest_row, ii, x);

  if(min == 0.0) return coeff_zero();

  return coeff_times(calcUtils_totProbeMulteplicity(highest_row), min);
}

static void minIn__givenProbe(row_t highest_row, coeff_t prev_lowest_curr[NUM_NORND_COLS], coeff_t *ret_lowest_max, coeff_t ret_lowest_curr[NUM_NORND_COLS]){
  // ii = 0, to init the variables of the cycle
  *ret_lowest_max = coeff_zero();
  for(col_t x = 0; x < NUM_NORND_COLS; x++){
    ret_lowest_curr[x] = coeff_add(prev_lowest_curr[x], toBeSummed(highest_row, 0, x));
    *ret_lowest_max = coeff_max(*ret_lowest_max, ret_lowest_curr[x]);
  }

  for(int ii = 1; ii < NUM_NORND_COLS; ii++){ // try every result.
    if(calcUtils_maxSharesIn(ii) > T) continue;

    coeff_t curr[NUM_NORND_COLS]; // over x
    coeff_t max = coeff_zero();
    for(col_t x = 0; x < NUM_NORND_COLS; x++){
      curr[x] = coeff_add(prev_lowest_curr[x], toBeSummed(highest_row, ii, x));
      max = coeff_max(max, curr[x]);
    }

    bool used_best;
    bool used_curr;
    *ret_lowest_max = coeff_min_and_usage(*ret_lowest_max, max, &used_best, &used_curr);

    if(used_best && used_curr){ // it leads to a higher f than the actual on, so it's a fine approximation.
      for(shift_t x = 0; x < NUM_NORND_COLS; x++)
        ret_lowest_curr[x] = coeff_max(ret_lowest_curr[x], curr[x]);
    }
    if(used_curr)
      for(shift_t x = 0; x < NUM_NORND_COLS; x++)
        ret_lowest_curr[x] = curr[x];
  }
}


// TODO: WARNING: approximated ii by finding a greedy (less-then-local) minimum, one probe at a time without going back. the result respects the max_x and is higher than min_in
static coeff_t minIn(row_t out){
  coeff_t prev_lowest_curr[NUM_NORND_COLS]; // over x
  coeff_t prev_lowest_max = coeff_zero();
  for(col_t x = 0; x < NUM_NORND_COLS; x++)
    prev_lowest_curr[x] = coeff_zero();

  row_t probes = row_first();
  do{
    coeff_t lowest_max;
    coeff_t lowest_curr[NUM_NORND_COLS]; // over x

    minIn__givenProbe(row_or(probes, out), prev_lowest_curr, &lowest_max, lowest_curr);

    prev_lowest_max = lowest_max;
    for(col_t x = 0; x < NUM_NORND_COLS; x++)
      prev_lowest_curr[x] = lowest_curr[x];
  }while(row_tryNextProbe(& probes));

  return prev_lowest_max;
}


coeff_t calc_rpcTeo(void){
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(double), row_size * NUM_NORND_COLS * NUM_NORND_COLS,  "rowData for calc_rpcTeo");
  for(col_t ii = 0; ii < NUM_NORND_COLS; ii++)
    if(calcUtils_maxSharesIn(ii) <= T)
      for(hash_s_t i = 0; i < row_size; i++)
        for(col_t x = 0; x < NUM_NORND_COLS; x++)
          *data_get(rowData, i, ii, x, row_size) = NAN;

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_sum = mem_calloc(sizeof(double), probe_size * NUM_NORND_COLS * NUM_NORND_COLS, "probeData_sum for calc_rpcTeo");
  for(col_t ii = 0; ii < NUM_NORND_COLS; ii++)
    if(calcUtils_maxSharesIn(ii) <= T)
      for(hash_s_t i = 0; i < probe_size; i++)
        for(col_t x = 0; x < NUM_NORND_COLS; x++)
          *data_get(probeData_sum, i, ii, x, probe_size) = NAN;

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_min = mem_calloc(sizeof(double), probe_size * NUM_NORND_COLS * NUM_NORND_COLS, "probeData_min for calc_rpcTeo");
  for(col_t ii = 0; ii < NUM_NORND_COLS; ii++)
    if(calcUtils_maxSharesIn(ii) <= T)
      for(hash_s_t i = 0; i < probe_size; i++)
        for(col_t x = 0; x < NUM_NORND_COLS; x++)
          *data_get(probeData_min, i, ii, x, probe_size) = NAN;

  coeff_t ret = coeff_zero();
  row_t output = row_first();
  do{
    coeff_t curr = minIn(output);
    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));

  mem_free(rowData);
  mem_free(probeData_sum);
  mem_free(probeData_min);

  return ret;
}
