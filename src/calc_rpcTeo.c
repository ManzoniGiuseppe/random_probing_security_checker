#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
static fixed_probesum_t *probeData_sum;
static double *probeData_min;
static size_t row_size;
static size_t probe_size;

#define MAX_O_COMBS (1ll << (MAX_COEFF + T*NUM_OUTS))

static inline fixed_rowsum_t* rowData_get(hash_s_t row, int ii_index, col_t x){
  return & rowData[x + NUM_NORND_COLS * (row + row_size * ii_index)];
}

static inline fixed_probesum_t* probeData_sum_get(hash_s_t row, int ii_index, col_t x, int o_index){
  return & probeData_sum[x + NUM_NORND_COLS * (row + probe_size * ii_index + probe_size * II_USED_COMB * o_index)];
}

static inline double* probeData_min_get(hash_s_t row, int ii_index, col_t x){
  return & probeData_min[x + NUM_NORND_COLS * (row + probe_size * ii_index)];
}

static int xor_col(col_t v1, col_t v2){
  col_t v = v1 & v2;
  shift_t ones = __builtin_popcountll(v);
  return (ones % 2 == 0) ? 1 : -1;
}

static void rowData_init(row_t row, col_t ii, int ii_index, col_t x){
  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  fixed_rowsum_t *it = rowData_get(rowTransform_transform_hash(row), ii_index, x);

  *it = 0.0;
  for(col_t i = 0; i < NUM_NORND_COLS; i++){
    if((i &~ ii) != 0){
      *it += transform[i] * xor_col(i, x);
    }
  }
}

static fixed_rowsum_t rowData_sumPhase(row_t row, int ii_index, col_t x){
  return *rowData_get(rowTransform_transform_hash(row), ii_index, x);
}

static int xor_row(row_t v1, row_t v2){
  row_t v = row_and(v1, v2);
  int ones = row_numOnes(v);
  return (ones % 2 == 0) ? 1 : -1;
}

// without multeplicity
static void probeData_sum_init(row_t row, row_t o, int o_index, __attribute__((unused)) col_t ii, int ii_index, col_t x){
  fixed_probesum_t *it = probeData_sum_get(rowTransform_row_hash(row), ii_index, x, o_index);

  *it = 0.0;
  row_t omega = row_first();
  do{
     *it += rowData_sumPhase(omega, ii_index, x) * xor_row(o, omega);
  }while(row_tryGetNext(row, & omega));
}

static double probeData_sumPhase(row_t row, int o_index, int ii_index, col_t x){
  return *probeData_sum_get(rowTransform_row_hash(row), ii_index, x, o_index) / (double) (1ll << NUM_TOT_INS);
}

static void probeData_min_init(row_t row, __attribute__((unused)) col_t ii, int ii_index, col_t x){
  double *it = probeData_min_get(rowTransform_row_hash(row), ii_index, x);

  *it = 0.0;
  row_t o = row_first();
  int o_index = 0;
  do{
    double val = probeData_sumPhase(row, o_index, ii_index, x);
    *it += ABS(val);
    o_index++;
  }while(row_tryGetNext(row, & o));

  *it = *it / 2 * ldexp(1.0, - row_numOnes(row)); // 2 ** - row_numOnes(row) = multeplicity * 2** - numprobes
  *it = MIN(1, *it);
}

static double probeData_evalMin(row_t row, int ii_index, col_t x){
  return *probeData_min_get(rowTransform_row_hash(row), ii_index, x);
}

// with multeplicity
static coeff_t toBeSummed(row_t highest_row, int ii_index, col_t x){
  double min = probeData_evalMin(highest_row, ii_index, x);

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

  int ii_index = 0;
  for(int ii = 0; ii < NUM_NORND_COLS; ii++){ // try every result.
    if(calcUtils_maxSharesIn(ii) > T) continue;

    coeff_t curr[NUM_NORND_COLS]; // over x
    coeff_t max = coeff_zero();
    for(col_t x = 0; x < NUM_NORND_COLS; x++){
      curr[x] = coeff_add(prev_lowest_curr[x], toBeSummed(highest_row, ii_index, x));
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
    ii_index++;
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
  printf("rpcTeo: 0/4\n");
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(fixed_rowsum_t), row_size * II_USED_COMB * NUM_NORND_COLS,  "rowData for calc_rpcTeo");
  calcUtils_init_outIiX(1, rowData_init);
  printf("rpcTeo: 1/4\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_sum = mem_calloc(sizeof(fixed_probesum_t), probe_size * II_USED_COMB * MAX_O_COMBS * NUM_NORND_COLS, "probeData_sum for calc_rpcTeo");
  calcUtils_init_outOIiX(0, probeData_sum_init);
  mem_free(rowData);
  printf("rpcTeo: 2/4\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_min = mem_calloc(sizeof(double), probe_size * II_USED_COMB * NUM_NORND_COLS, "probeData_min for calc_rpcTeo");
  calcUtils_init_outIiX(0, probeData_min_init);
  mem_free(probeData_sum);
  printf("rpcTeo: 3/4\n");

  coeff_t ret = coeff_zero();
  row_t output = row_first();
  do{
    coeff_t curr = minIn(output);
    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));

  mem_free(probeData_min);
  printf("rpcTeo: 4/4\n");
  return ret;
}
