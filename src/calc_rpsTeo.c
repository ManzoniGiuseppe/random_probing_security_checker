#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"



// fixed point notation (1+NUM_INS).NUM_TOT_INS
#if NUM_TOT_INS+1 + NUM_INS <= 16-1
  typedef int16_t fixed_rowsum_t;
#elif NUM_TOT_INS+1 + NUM_INS <= 32-1
  typedef int32_t fixed_rowsum_t;
#elif NUM_TOT_INS+1 + NUM_INS <= 64-1
  typedef int64_t fixed_rowsum_t;
#else
  typedef double fixed_rowsum_t;
#endif

// fixed point notation (1+NUM_INS + MAX_COEFF).NUM_TOT_INS
#if NUM_TOT_INS+1 + NUM_INS + MAX_COEFF <= 16-1
  typedef int16_t fixed_probesum_t;
#elif NUM_TOT_INS+1 + NUM_INS + MAX_COEFF <= 32-1
  typedef int32_t fixed_probesum_t;
#elif NUM_TOT_INS+1 + NUM_INS + MAX_COEFF <= 64-1
  typedef int64_t fixed_probesum_t;
#else
  typedef double fixed_probesum_t;
#endif



static fixed_rowsum_t *rowData;
static double *probeData_min;
static size_t row_size;
static size_t probe_size;

#define MAX_O_COMBS (1ll << MAX_COEFF)

static inline fixed_rowsum_t* rowData_get(row_t row, col_t x){
  return & rowData[rowTransform_transform_hash(row) + row_size * x];
}

static inline double* probeData_min_get(row_t row, col_t x){
  return & probeData_min[rowTransform_row_hash(row) + probe_size * x];
}


static int xor_col(col_t v1, col_t v2){
  col_t v = v1 & v2;
  shift_t ones = __builtin_popcountll(v);
  return (ones % 2 == 0) ? 1 : -1;
}

static void rowData_init(row_t row, col_t x){
  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  fixed_rowsum_t *it = rowData_get(row, x);

  *it = 0.0;
  for(col_t i = 1; i < 1ll<<NUM_INS; i++){ // 0 excluded
    *it += transform[calcUtils_intExpandByD(i)] * xor_col(i, x);
  }
}

static fixed_rowsum_t rowData_sumPhase(row_t row, col_t x){
  return *rowData_get(row, x);
}

static int xor_row(row_t v1, row_t v2){
  row_t v = row_and(v1, v2);
  int ones = row_numOnes(v);
  return (ones % 2 == 0) ? 1 : -1;
}

// without multeplicity
static double probeData_sumPhase(row_t row, row_t o, col_t x){
  fixed_probesum_t it = 0.0;
  row_t omega = row_first();
  do{
     it += rowData_sumPhase(omega, x) * xor_row(o, omega);
  }while(row_tryGetNext(row, & omega));

  return it / (double) (1ll << NUM_TOT_INS);
}

static void probeData_min_init(row_t row, col_t x){
  double *it = probeData_min_get(row, x);

  *it = 0.0;
  row_t o = row_first();
  do{
    double val = probeData_sumPhase(row, o, x);
    *it += ABS(val);
  }while(row_tryGetNext(row, & o));

  *it = *it / 2 * ldexp(1.0, - row_numOnes(row)); // 2 ** - row_numOnes(row) = multeplicity * 2** - numprobes
  *it = MIN(1, *it);
}

static double probeData_evalMin(row_t row, col_t x){
  return *probeData_min_get(row, x);
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
  printf("rpsTeo: 0/3\n");
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(double), row_size * (1ll << NUM_INS),  "rowData for calc_rpsTeo");
  ITERATE_PROBE(1, {
    ITERATE_X({
      rowData_init(probe, x);
    })
  })
  printf("rpsTeo: 1/3\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_min = mem_calloc(sizeof(double), probe_size * (1ll << NUM_INS), "probeData_min for calc_rpsTeo");
  ITERATE_PROBE(0, {
    ITERATE_X({
      probeData_min_init(probe, x);
    })
  })
  mem_free(rowData);
  printf("rpsTeo: 2/3\n");

  coeff_t max = coeff_zero();
  for(col_t x = 0; x < 1ll<<NUM_INS; x++){
    max = coeff_max(max, sumProbes(x));
  }

  mem_free(probeData_min);
  printf("rpsTeo: 3/3\n");
  return max;
}
