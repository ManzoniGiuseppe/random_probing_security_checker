#include <stdio.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


static inline bool* data_get(uint8_t *data, hash_s_t row){
  return & data[row];
}

static bool *rowData;
static bool *probeDataOut;
static bool *probeData;
static size_t row_size;
static size_t probe_size;

static void rowData_init(row_t row){
  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  bool *anyNot0 = data_get(rowData, rowTransform_row_hash(row));

  for(int i = 0; i < NUM_NORND_COLS; i++){
    if(calcUtils_maxSharesIn(i) > T && transform[i] != 0){
      *anyNot0 = 1;
      return;
    }
  }

  *anyNot0 = 0;
}

static bool rowData_anyNot0(row_t row){
  return *data_get(rowData, rowTransform_row_hash(row));
}

static void probeDataOut_init(row_t probe){
  bool *anyNot0 = data_get(probeDataOut, rowTransform_unique_hash(probe));

  row_t output = row_first();
  do{
    if(rowData_anyNot0(row_or(output,probe))){
      *anyNot0 = 1;
      return;
    }
  }while(row_tryNextOut(& output));

  *anyNot0 = 0;
}

static bool probeDataOut_anyNot0(row_t probe){
  return *data_get(probeDataOut, rowTransform_unique_hash(probe));
}


static bool probeData_anyNot0(row_t probe);
static void probeData_init(row_t probe){
  bool *anyNot0 = data_get(probeData, rowTransform_unique_hash(probe));

  if(probeDataOut_anyNot0(probe)){
    *anyNot0 = 1;
    return;
  }

  /* check if any of the direct sub-probes has the 1, if so then this is too */
  ITERATE_OVER_DIRECT_SUB_ROWS(probe, sub, {
    if(probeData_anyNot0(sub)){
      *anyNot0 = 1;
      return;
    }
  })

  *anyNot0 = 0;
}

static bool probeData_anyNot0(row_t probe){
  return *data_get(probeData, rowTransform_unique_hash(probe));
}

static bool probeData_is(row_t probe){
  return probeData_anyNot0(probe);
}


coeff_t calc_rpcW(void){
  printf("rpcIs: 0/4\n");
  row_size = rowTransform_row_hash_size();
  probe_size = rowTransform_unique_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(bool), row_size, "rowData for calc_rpcIs");
  ITERATE_PROBE_AND_OUT(IPT_ROW, {
    rowData_init(probeAndOut);
  })
  printf("rpcIs: 1/4\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeDataOut = mem_calloc(sizeof(bool), probe_size, "probeData for calc_rpcIs");
  ITERATE_PROBE(IPT_UNIQUE, {
    probeDataOut_init(probe);
  })
  mem_free(rowData);
  printf("rpcIs: 2/4\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData = mem_calloc(sizeof(bool), probe_size, "probeData for calc_rpcIs");
  ITERATE_PROBE(IPT_UNIQUE, {
    probeData_init(probe);
  })
  mem_free(probeDataOut);
  printf("rpcIs: 3/4\n");

  coeff_t ret = coeff_zero();

  row_t probe = row_first();
  do{
    bool is = probeData_is(probe);
    if(is != 0)
      ret = coeff_add(ret, calcUtils_totProbeMulteplicity(probe));
  }while(row_tryNextProbe(& probe));

  mem_free(probeData);
  printf("rpcIs: 4/4\n");
  return ret;
}

