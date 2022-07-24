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
  for(int j = 0; j < ROW_VALUES_SIZE; j++){
    row_value_t row_it = row.values[j];
    if(row_it == 0) continue;

    for(shift_t i = TAIL_1(row_it); i <= LEAD_1(row_it); i++){
      row_value_t sub_it = row_it &~ (1ll << i);
      if(sub_it == row_it) continue; /* it's not a sub-probe */
      row_t sub = row;
      sub.values[j] = sub_it;

      if(probeData_anyNot0(probeData, rowData, sub)){
        *it = ANY_NOT_0;
        return 1;
      }
    }
  }

  *it = ALL_0;
  return 0;
}


coeff_t calc_rpsIs(void){
  // to store if the wanted row as any != 0 in the appropriate columns.
  uint8_t* rowData = mem_calloc(sizeof(uint8_t), 1ll << ROWTRANSFORM_RESERVATION_BITS, "rowData for calc_rpsIs");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  uint8_t* probeData = mem_calloc(sizeof(uint8_t), 1ll << ROWTRANSFORM_ASSOC_BITS, "probeData for calc_rpsIs");

  coeff_t ret = coeff_zero();

  ITERATE_OVER_PROBES(probes, coeffNum, {
    row_t row = probeComb_getHighestRow_noOut(probes); // TODO: fix, iterate over row_t.
    bool is = probeData_anyNot0(probeData, rowData, row);
    if(is != 0){
      double mul = probeComb_getProbesMulteplicity(probes);
      ret.values[coeffNum] += is * mul;
    }
  })

  mem_free(rowData);
  mem_free(probeData);

  return ret;
}
