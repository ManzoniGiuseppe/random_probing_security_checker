#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


#define  UNINIT     0
#define  ALL_0      1
#define  ANY_NOT_0  2


static inline uint8_t* data_get(uint8_t *data, hash_s_t row, col_t ii, hash_s_t size){
  return & data[row + ii * size];
}

static uint8_t *rowData;
static uint8_t *probeData;
static hash_s_t row_size;
static hash_s_t probe_size;

static bool rowData_anyNot0(row_t row, col_t ii){
  uint8_t *it = data_get(rowData, rowTransform_transform_hash(row), ii, row_size);
  if(*it) return *it == ANY_NOT_0; // inited

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  for(int i = 0; i < NUM_NORND_COLS; i++){
    if((i &~ ii) != 0 && transform[i] != 0){
      *it = ANY_NOT_0;
      return 1;
    }
  }
  *it = ALL_0;
  return 0;
}

static bool probeData_anyNot0(row_t row, col_t ii){
  uint8_t *it = data_get(probeData, rowTransform_row_hash(row), ii, probe_size);
  if(*it) return *it == ANY_NOT_0; // inited

  if(rowData_anyNot0(row, ii)){
    *it = ANY_NOT_0;
    return 1;
  }

  /* check if any of the direct sub-probes has the 1, if so then this is too */
  ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
    if(probeData_anyNot0(sub, ii)){
      *it = ANY_NOT_0;
      return 1;
    }
  })

  *it = ALL_0;
  return 0;
}

static bool probeData_is(row_t row){
  for(col_t ii = 0; ii < NUM_NORND_COLS; ii++){
    if(calcUtils_maxSharesIn(ii) <= T && !probeData_anyNot0(row, ii)){
      return 0; // all must be true
    }
  }
  return 1;
}




coeff_t calc_rpcIs(void){
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(uint8_t), row_size * NUM_NORND_COLS, "rowData for calc_rpcIs");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData = mem_calloc(sizeof(uint8_t), probe_size * NUM_NORND_COLS, "probeData for calc_rpcIs");

  coeff_t ret = coeff_zero();
  row_t output = row_first();
  do{
    if(row_maxShares(output) != T) continue; // the max will be in the ones with most outputs
    coeff_t curr = coeff_zero();

    row_t probe = row_first();
    do{
      row_t row = row_or(probe, output);
      bool is = probeData_is(row);
      if(is != 0){
        ITERATE_OVER_PROBES_WHOSE_IMAGE_IS(row, probeComb, coeffNum, {
          double mul = probeComb_getProbesMulteplicity(probeComb);
          curr.values[coeffNum] += mul;
        })
      }
    }while(row_tryNextProbe(& probe));

    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));


  mem_free(rowData);
  mem_free(probeData);

  return ret;
}

