#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "hashSet.h"
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
static hash_s_t *probeData_min;
static size_t row_size;
static size_t probe_size;

typedef struct {
  hash_l_t hash; // mandatory here.
  double info[NUM_NORND_COLS][II_USED_COMB];
} probe_info_t;

static hashSet_t htProbe;



static inline fixed_rowsum_t* rowData_get(row_t row, int ii_index, col_t x){
  return & rowData[x + NUM_NORND_COLS * (rowTransform_transform_hash(row) + row_size * ii_index)];
}

static inline hash_s_t* probeData_min_get(row_t row){
  return & probeData_min[rowTransform_row_hash(row)];
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

static fixed_rowsum_t rowData_sumPhase(row_t row, int ii_index, col_t x){
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
  row_t omega = row_first();
  do{
     it += rowData_sumPhase(omega, ii_index, x) * xor_row(o, omega);
  }while(row_tryGetNext(row, & omega));

  return it / (double) (1ll << NUM_TOT_INS);
}

static double probeData_min_init__i(row_t row, __attribute__((unused)) col_t ii, int ii_index, col_t x){
  double ret = 0.0;
  row_t o = row_first();
  do{
    double val = probeData_sumPhase(row, o, ii_index, x);
    ret += ABS(val);
  }while(row_tryGetNext(row, & o));

  ret = ret / 2 * ldexp(1.0, - row_numOnes(row)); // 2 ** - row_numOnes(row) = multeplicity * 2** - numprobes
  ret = MIN(1, ret);
  return ret;
}


static void increaseSizeHtProbe(void){
  hashSet_t htProbe_new = hashSet_new(hashSet_getNumElem(htProbe) + 1, sizeof(probe_info_t), "htProbe for calc_rpcTeo");
  hash_s_t *newPos = mem_calloc(sizeof(hash_s_t), 1ll << hashSet_getNumElem(htProbe), "htProbe translator for calc_rpcTeo");

  for(hash_s_t i = 0; i < 1ll<<hashSet_getNumElem(htProbe); i++)
    if(hashSet_validPos(htProbe, i))
      if(!hashSet_tryAdd(htProbe_new, hashSet_getKey(htProbe, i), newPos + i))
        FAIL("calc_rpcTeo: couldn't add a row to the htProbe_new! fill=%f, conflictRate=%f\n", hashSet_dbg_fill(htProbe_new), hashSet_dbg_hashConflictRate(htProbe_new));

  hashSet_delete(htProbe);
  htProbe = htProbe_new;

  for(hash_s_t i = 0; i < probe_size; i++){
    hash_s_t *it = & probeData_min[i];
    *it = newPos[*it];
  }

  mem_free(newPos);
}

static void probeData_min_init(row_t row){
  probe_info_t key;
  memset(&key, 0, sizeof(probe_info_t));

  ITERATE_II({
    ITERATE_X({
      key.info[x][ii_index] = probeData_min_init__i(row, ii, ii_index, x);
    })
  })

  hash_s_t *pos = probeData_min_get(row);
  if(!hashSet_tryAdd(htProbe, &key, pos)){
     increaseSizeHtProbe();
     if(!hashSet_tryAdd(htProbe, &key, pos)){
       FAIL("calc_rpcTeo: couldn't insert a probeData's info into the htProbe\n");
    }
  }
}



static void minIn__givenProbe(row_t highest_row, coeff_t prev_lowest_curr[NUM_NORND_COLS], coeff_t *ret_lowest_max, coeff_t ret_lowest_curr[NUM_NORND_COLS]){
  probe_info_t *highest_row_info = hashSet_getKey(htProbe, *probeData_min_get(highest_row));
  coeff_t multeplicity = calcUtils_totProbeMulteplicity(highest_row);

  // ii = 0, to init the variables of the cycle
  *ret_lowest_max = coeff_zero();
  for(col_t x = 0; x < NUM_NORND_COLS; x++){
    ret_lowest_curr[x] = coeff_add(prev_lowest_curr[x], coeff_times(multeplicity, highest_row_info->info[x][0]));
    *ret_lowest_max = coeff_max(*ret_lowest_max, ret_lowest_curr[x]);
  }

  int ii_index = 0;
  for(int ii = 0; ii < NUM_NORND_COLS; ii++){ // try every result.
    if(calcUtils_maxSharesIn(ii) > T) continue;

    coeff_t curr[NUM_NORND_COLS]; // over x
    coeff_t max = coeff_zero();
    for(col_t x = 0; x < NUM_NORND_COLS; x++){
      curr[x] = coeff_add(prev_lowest_curr[x], coeff_times(multeplicity, highest_row_info->info[x][ii_index]));
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
  printf("rpcTeo: 0/3\n");
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(fixed_rowsum_t), row_size * II_USED_COMB * NUM_NORND_COLS,  "rowData for calc_rpcTeo");
  ITERATE_PROBE_AND_OUT(1, {
    ITERATE_II({
      ITERATE_X({
        rowData_init(probeAndOutput, ii, ii_index, x);
      })
    })
  })
  printf("rpcTeo: 1/3\n");

  htProbe = hashSet_new(RPCTEO_HTPROBE_BITS, sizeof(probe_info_t), "htProbe for calc_rpcTeo");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData_min = mem_calloc(sizeof(hash_s_t), probe_size, "probeData_min for calc_rpcTeo");
  ITERATE_PROBE_AND_OUT(0, {
    probeData_min_init(probeAndOutput);
  })
  mem_free(rowData);
  printf("rpcTeo: 2/3\n");

  coeff_t ret = coeff_zero();
  row_t output = row_first();
  do{
    coeff_t curr = minIn(output);
    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));

  mem_free(probeData_min);
  hashSet_delete(htProbe);
  printf("rpcTeo: 3/3\n");
  return ret;
}
