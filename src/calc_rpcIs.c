#include <stdio.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


static inline bool* data_get(uint8_t *data, hash_s_t row, int ii_index, hash_s_t size){
  return & data[row + ii_index * size];
}

static bool *rowData;
static bool *probeData;
static size_t row_size;
static size_t probe_size;

static void rowData_init(row_t row, col_t ii, int ii_index){
  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  bool *anyNot0 = data_get(rowData, rowTransform_transform_hash(row), ii_index, row_size);

  for(int i = 0; i < NUM_NORND_COLS; i++){
    if((i &~ ii) != 0 && transform[i] != 0){
      *anyNot0 = 1;
      return;
    }
  }

  *anyNot0 = 0;
}

static bool rowData_anyNot0(row_t row, int ii_index){
  return *data_get(rowData, rowTransform_transform_hash(row), ii_index, row_size);
}

static bool probeData_anyNot0(row_t row, int ii_index);
static void probeData_init(row_t row, int ii_index){
  bool *anyNot0 = data_get(probeData, rowTransform_row_hash(row), ii_index, probe_size);

  if(rowData_anyNot0(row, ii_index)){
    *anyNot0 = 1;
    return;
  }

  /* check if any of the direct sub-probes has the 1, if so then this is too */
  ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
    if(probeData_anyNot0(sub, ii_index)){
      *anyNot0 = 1;
      return;
    }
  })

  *anyNot0 = 0;
}

static bool probeData_anyNot0(row_t row, int ii_index){
  return *data_get(probeData, rowTransform_row_hash(row), ii_index, probe_size);
}

static bool probeData_is(row_t row){
  int ii_index = 0;
  for(col_t ii = 0; ii < NUM_NORND_COLS; ii++){
    if(calcUtils_maxSharesIn(ii) <= T){
      if(!probeData_anyNot0(row, ii_index)){
        return 0; // all must be true
      }
      ii_index++;
    }
  }
  return 1;
}


coeff_t calc_rpcIs(void){
  printf("rpcIs: 0/3\n");
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(bool), row_size * II_USED_COMB, "rowData for calc_rpcIs");
  ITERATE_PROBE_AND_OUT(1, {
    ITERATE_II({
        rowData_init(probeAndOutput, ii, ii_index);
    })
  })
  printf("rpcIs: 1/3\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData = mem_calloc(sizeof(bool), probe_size * II_USED_COMB, "probeData for calc_rpcIs");
  ITERATE_PROBE_AND_OUT(0, {
    ITERATE_II({
        probeData_init(probeAndOutput, ii_index);
    })
  })
  mem_free(rowData);
  printf("rpcIs: 2/3\n");

  coeff_t ret = coeff_zero();
  row_t output = row_first();
  do{
    if(row_maxShares(output) != T) continue; // the max will be in the ones with most outputs
    coeff_t curr = coeff_zero();

    row_t probe = row_first();
    do{
      row_t row = row_or(probe, output);
      bool is = probeData_is(row);
      if(is != 0)
        curr = coeff_add(curr, calcUtils_totProbeMulteplicity(row));
    }while(row_tryNextProbe(& probe));

    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));

  mem_free(probeData);
  printf("rpcIs: 3/3\n");
  return ret;
}

