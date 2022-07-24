#include "calc.h"
#include "row.h"
#include "mem.h"
#include "probeComb.h"
#include "rowTransform.h"


#define  UNINIT     0
#define  ALL_0      1
#define  ANY_NOT_0  2


static inline uint8_t* data_get(uint8_t *data, hash_t row, col_t ii, shift_t bits){
  return & data[row + ii * (1ll << bits)];
}

static bool rowData_anyNot0(uint8_t *rowData, row_t row, col_t ii){
  uint8_t *it = data_get(rowData, rowTransform_transform_hash(row), ii, ROWTRANSFORM_RESERVATION_BITS);
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

static bool probeData_anyNot0(uint8_t* probeData, uint8_t *rowData, row_t row, col_t ii){
  uint8_t *it = data_get(probeData, rowTransform_row_hash(row), ii, ROWTRANSFORM_ASSOC_BITS);
  if(*it) return *it == ANY_NOT_0; // inited

  if(rowData_anyNot0(rowData, row, ii)){
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

      if(probeData_anyNot0(probeData, rowData, sub, ii)){
        *it = ANY_NOT_0;
        return 1;
      }
    }
  }

  *it = ALL_0;
  return 0;
}

static shift_t maxShares_in(col_t value){
  shift_t ret = 0;
  col_t mask = (1ll << D)-1;
  for(int i = 0; i < NUM_INS; i++){
    int v = __builtin_popcountll(value & (mask << (i*D)));
    ret = v > ret ? v : ret;
  }
  return ret;
}


static bool probeData_is(uint8_t *probeData, uint8_t *rowData, row_t row){
  for(col_t ii = 0; ii < NUM_NORND_COLS; ii++){
    if(maxShares_in(ii) <= T && !probeData_anyNot0(probeData, rowData, row, ii)){
      return 0; // all must be true
    }
  }
  return 1;
}




coeff_t calc_rpcIs(void){
  // to store if the wanted row as any != 0 in the appropriate columns.
  uint8_t* rowData = mem_calloc(sizeof(uint8_t), (1ll << ROWTRANSFORM_RESERVATION_BITS) * NUM_NORND_COLS, "rowData for calc_rpcIs");

  // like for the row, but it acts on any sub-row, capturing the whole probe.
  uint8_t* probeData = mem_calloc(sizeof(uint8_t), (1ll << ROWTRANSFORM_ASSOC_BITS) * NUM_NORND_COLS, "probeData for calc_rpcIs");

  coeff_t ret = coeff_zero();


  row_t output = row_first();
  do{
    if(row_maxShares(output) != T) continue; // the max will be in the ones with most outputs
    coeff_t curr = coeff_zero();

    ITERATE_OVER_PROBES(probes, coeffNum, {
      row_t row = probeComb_getHighestRow(probes, output);
      bool is = probeData_is(probeData, rowData, row);
      if(is != 0){
        double mul = probeComb_getProbesMulteplicity(probes);
        curr.values[coeffNum] += is * mul;
      }
    })

    ret = coeff_max(ret, curr);
  }while(row_tryNextOut(& output));


  mem_free(rowData);
  mem_free(probeData);

  return ret;
}
