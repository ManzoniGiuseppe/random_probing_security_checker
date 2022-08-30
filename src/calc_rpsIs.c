#include <stdio.h>

#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


static bool *rowData;
static bool *probeData;
static size_t row_size;
static size_t probe_size;

static void rowData_init(row_t row){
  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  bool *anyNot0 = & rowData[rowTransform_transform_hash(row)];

  for(int i = 1; i < (1ll<<NUM_INS); i++){
    if(transform[calcUtils_intExpandByD(i)] != 0){
      *anyNot0 = 1;
      return;
    }
  }
  *anyNot0 = 0;
  return;
}

static bool rowData_anyNot0(row_t row){
  return rowData[rowTransform_transform_hash(row)];
}

static bool probeData_anyNot0(row_t row);
static void probeData_init(row_t row){
  bool *anyNot0 = & probeData[rowTransform_row_hash(row)];

  if(rowData_anyNot0(row)){
    *anyNot0 = 1;
    return;
  }

  /* check if any of the direct sub-probes has the 1, if so then this is too */
  ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
    if(probeData_anyNot0(sub)){
      *anyNot0 = 1;
      return;
    }
  })

  *anyNot0 = 0;
  return;
}

static bool probeData_anyNot0(row_t row){
  return probeData[rowTransform_row_hash(row)];
}


coeff_t calc_rpsIs(void){
  printf("rpsIs: 0/3\n");
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(bool), row_size, "rowData for calc_rpsIs");
  ITERATE_PROBE(1, {
    rowData_init(probe);
  })
  printf("rpsIs: 1/3\n");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData = mem_calloc(sizeof(bool), probe_size, "probeData for calc_rpsIs");
  ITERATE_PROBE(0, {
    probeData_init(probe);
  })
  mem_free(rowData);
  printf("rpsIs: 2/3\n");

  coeff_t ret = coeff_zero();
  row_t row = row_first();
  do{
    bool is = probeData_anyNot0(row);
    if(is != 0){
      ret = coeff_add(ret, calcUtils_totProbeMulteplicity(row));
    }
  }while(row_tryNextProbe(& row));

  mem_free(probeData);
  printf("rpsIs: 3/3\n");
  return ret;
}
