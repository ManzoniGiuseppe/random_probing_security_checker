#include "calc.h"
#include "row.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


#define  UNINIT     0
#define  ALL_0      1
#define  ANY_NOT_0  2



// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
static col_t intExpandByD(col_t val){
  if(val == 0) return 0;
  col_t bit = (1ll<<D)-1;
  col_t ret = 0;
  for(shift_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << (i*D);
  }
  return ret;
}


static bool rowData_anyNot0(uint8_t *rowData, row_t row){
  uint8_t *it = & rowData[rowTransform_transform_hash(row)];
  if(*it) return *it == ANY_NOT_0; // inited

  fixed_cell_t transform[NUM_NORND_COLS];
  rowTransform_get(row, transform);

  for(int i = 1; i < (1ll<<NUM_INS); i++){
    if(transform[intExpandByD(i)] != 0){
      *it = ANY_NOT_0;
      return 1;
    }
  }
  *it = ALL_0;
  return 0;
}

static bool probeData_anyNot0(uint8_t* probeData, uint8_t *rowData, row_t row){
  uint8_t *it = & probeData[rowTransform_row_hash(row)];
  if(*it) return *it == ANY_NOT_0; // inited

  if(rowData_anyNot0(rowData, row)){
    *it = ANY_NOT_0;
    return 1;
  }

  /* check if any of the direct sub-probes has the 1, if so then this is too */
  ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
    if(probeData_anyNot0(probeData, rowData, sub)){
      *it = ANY_NOT_0;
      return 1;
    }
  })

  *it = ALL_0;
  return 0;
}


coeff_t calc_rpsIs(void){
  // to store if the wanted row as any != 0 in the appropriate columns.
  uint8_t* rowData = mem_calloc(sizeof(uint8_t), 1ll << ROWTRANSFORM_TRANSFORM_BITS, "rowData for calc_rpsIs");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  uint8_t* probeData = mem_calloc(sizeof(uint8_t), 1ll << ROWTRANSFORM_ROW_BITS, "probeData for calc_rpsIs");

  coeff_t ret = coeff_zero();
  row_t row = row_first();
  do{
    bool is = probeData_anyNot0(probeData, rowData, row);
    if(is != 0){
      ITERATE_OVER_PROBES_WHOSE_IMAGE_IS(row, probeComb, coeffNum, {
        double mul = probeComb_getProbesMulteplicity(probeComb);
        ret.values[coeffNum] += mul;
      })
    }
  }while(row_tryNextProbe(& row));

  mem_free(rowData);
  mem_free(probeData);

  return ret;
}
