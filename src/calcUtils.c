#include "calcUtils.h"
#include "probeComb.h"
#include "mem.h"
#include "row.h"
#include "rowTransform.h"


// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
col_t calcUtils_intExpandByD(col_t val){
  if(val == 0) return 0;
  col_t bit = (1ll<<D)-1;
  col_t ret = 0;
  for(shift_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << (i*D);
  }
  return ret;
}

coeff_t calcUtils_totProbeMulteplicity(row_t row){
  coeff_t ret = coeff_zero();
  ITERATE_OVER_PROBES_WHOSE_IMAGE_IS(row, probeComb, coeffNum, {
    double mul = probeComb_getProbesMulteplicity(probeComb);
    ret.values[coeffNum] += mul;
  })
  return ret;
}

shift_t calcUtils_maxSharesIn(col_t value){
  shift_t ret = 0;
  col_t mask = (1ll << D)-1;
  for(int i = 0; i < NUM_INS; i++){
    int v = __builtin_popcountll(value & (mask << (i*D)));
    ret = v > ret ? v : ret;
  }
  return ret;
}
