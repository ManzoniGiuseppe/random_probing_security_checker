#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"
#include "correlationTable.h"


#if NUM_TOT_OUTS > 64
  #error "probeComb_prot1 only supports NUM_TOT_OUTS <= 64"
#endif
#if NUM_OUTS != 1
  #error "probeComb_prot1 only supports NUM_OUTS == 1"
#endif



#if D * NUM_OUTS > 64
  #error "Current algprithm doesn't support D * NUM_OUTS > 64"
#endif
#if D * NUM_INS > 64
  #error "Current typing doesn't support D * NUM_INS > 64"
#endif



// useful def
#define LEAD_1(x) (63 - __builtin_clzll((x)))
#define TAIL_1(x) LEAD_1((x)&-(x))
#define MAX(x, y)  ((x) > (y) ? (x) : (y))


// -- getRow

shift_t row_maxShares(row_t r){
  row_value_t out = r.values[0] & ((1ll << D)-1);
  return __builtin_popcountll(out);
}
row_t row_singleInput(shift_t input){
  return (row_t){ { 1ll << input } };
}
row_t row_first(){
  return (row_t){ {0} };
}
bool row_eq(row_t r0, row_t r1){
  return r0.values[0] == r1.values[0];
}
row_t row_or(row_t r0, row_t r1){
  r0.values[0] |= r1.values[0];
  return r0;
}
row_t row_and(row_t r0, row_t r1){
  r0.values[0] &= r1.values[0];
  return r0;
}
row_t row_xor(row_t r0, row_t r1){
  r0.values[0] ^= r1.values[0];
  return r0;
}
row_t row_not(row_t r){
  r.values[0] = ~r.values[0];
  return r;
}

bool row_tryNextOut(row_t *curr){
  row_value_t outMask = (1ll << D)-1;

  row_value_t outs = curr->values[0] & outMask;
  if(__builtin_popcountll(outs+1) <= T){ // if incrementing leads to a valid result, just increment it
    curr->values[0]++;
    return 1;
  }
  if(__builtin_popcountll(outs >> (D - T)) == T){
    curr->values[0] &= ~outMask; // reset output
    return 0; // all 1s are at the top -> no output conmbinations left
  }

  row_value_t v = (1ll << TAIL_1(outs));
  curr->values[0] += v;
  return 1;
}

bool row_tryNextProbeAndOut(row_t *curr){
  if(row_tryNextOut(curr)) return 1;

  // increment the combination of porbes
  if(__builtin_popcountll(curr->values[0]) == NUM_PROBES || MAX_COEFF == 0) return 0; // end as we covered all probes.

  row_value_t next = curr->values[0] + (1ll << D);  // just increment the probe combination
  if(__builtin_popcountll(next) <= MAX_COEFF){
    curr->values[0] = next;
    return 1;
  }

  curr->values[0] += 1ll << TAIL_1(curr->values[0]);
  if(TAIL_1(curr->values[0]) >= NUM_TOT_OUTS) return 0; // if overflows

  return 1;
}
