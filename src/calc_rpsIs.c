#include "calc.h"
#include "calcUtils.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"

#define  UNINIT     0
#define  ALL_0      1
#define  ANY_NOT_0  2


static uint8_t *rowData;
static uint8_t *probeData;
static size_t row_size;
static size_t probe_size;

static bool rowData_anyNot0(row_t row){
  uint8_t *it = & rowData[rowTransform_transform_hash(row)];
  if(*it) return *it == ANY_NOT_0; // inited

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  for(int i = 1; i < (1ll<<NUM_INS); i++){
    if(transform[calcUtils_intExpandByD(i)] != 0){
      *it = ANY_NOT_0;
      return 1;
    }
  }
  *it = ALL_0;
  return 0;
}

static bool probeData_anyNot0(row_t row){
  uint8_t *it = & probeData[rowTransform_row_hash(row)];
  if(*it) return *it == ANY_NOT_0; // inited

  if(rowData_anyNot0(row)){
    *it = ANY_NOT_0;
    return 1;
  }

  /* check if any of the direct sub-probes has the 1, if so then this is too */
  ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
    if(probeData_anyNot0(sub)){
      *it = ANY_NOT_0;
      return 1;
    }
  })

  *it = ALL_0;
  return 0;
}


coeff_t calc_rpsIs(void){
  row_size = rowTransform_transform_hash_size();
  probe_size = rowTransform_row_hash_size();

  // to store if the wanted row as any != 0 in the appropriate columns.
  rowData = mem_calloc(sizeof(uint8_t), row_size, "rowData for calc_rpsIs");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  probeData = mem_calloc(sizeof(uint8_t), probe_size, "probeData for calc_rpsIs");

  coeff_t ret = coeff_zero();
  row_t row = row_first();
  do{
    bool is = probeData_anyNot0(row);
    if(is != 0){
      ret = coeff_add(ret, calcUtils_totProbeMulteplicity(row));
    }
  }while(row_tryNextProbe(& row));

  mem_free(rowData);
  mem_free(probeData);

  return ret;
}
