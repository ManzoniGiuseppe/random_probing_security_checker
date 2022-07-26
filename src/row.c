#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"
#include "row.h"




#if D * NUM_OUTS > 64
  #error "Current algprithm doesn't support D * NUM_OUTS > 64"
#endif
#if D * NUM_INS > 64
  #error "Current typing doesn't support D * NUM_INS > 64"
#endif



// -- getRow

shift_t row_maxShares(row_t r){
  shift_t max = 0;
  row_value_t lowest = r.values[0];
  row_value_t mask = (1ll << D)-1;
  for(int o = 0; o < NUM_OUTS; o++){
    shift_t val = __builtin_popcountll(lowest & (mask << o*D));
    max = MAX(max, val);
  }
  return max;
}
row_t row_singleInput(shift_t input){
  row_t ret = (row_t){ {0} };
  ret.values[input/64] = 1ll << (input % 64);
  return ret;
}
row_t row_first(){
  return (row_t){ {0} };
}
bool row_eq(row_t r0, row_t r1){
  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    if(r0.values[i] != r1.values[i])
      return 0;
  }
  return 1;
}
row_t row_or(row_t r0, row_t r1){
  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    r0.values[i] |= r1.values[i];
  }
  return r0;
}
row_t row_and(row_t r0, row_t r1){
  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    r0.values[i] &= r1.values[i];
  }
  return r0;
}
row_t row_xor(row_t r0, row_t r1){
  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    r0.values[i] ^= r1.values[i];
  }
  return r0;
}
row_t row_not(row_t r){
  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    r.values[i] = ~r.values[i];
  }
  return r;
}

shift_t row_numOnes(row_t r){
  shift_t ret = 0;
  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    ret += __builtin_popcountll(r.values[i]);
  }
  return ret;
}

static inline row_t row_add(row_t r, uint64_t toAdd, shift_t from){
  for(int i = from; i < ROW_VALUES_SIZE && toAdd != 0; i++){
    r.values[i] += toAdd;
    toAdd = r.values[i] < toAdd ? 1 : 0; // carry if it overflows
  }
  return r;
}

static bool row_tryNextOut__i(row_t *curr, shift_t o){
  row_value_t outMask = ((1ll << D)-1) << o*D;
  row_value_t one = 1ll << o*D;

  row_value_t outs = curr->values[0] & outMask;
  if(__builtin_popcountll(outs >> (o*D + D-T)) == T){
    curr->values[0] &= ~outMask; // reset this out
    return 0; // all 1s are at the top -> no output conmbinations left
  }

  if(__builtin_popcountll(outs+one) <= T){ // if incrementing leads to a valid result, just increment it
    curr->values[0] += one;
  }else{
    row_value_t v = (1ll << TAIL_1(outs));
    curr->values[0] += v;
  }
  return 1;
}

bool row_tryNextOut(row_t *curr){
  for(int o = 0; o < NUM_OUTS; o++){
    if(row_tryNextOut__i(curr, o)) return 1;
    // the combination of output o has already been resetted
  }
  return 0; // automatically resets the output combination
}

bool row_tryNextProbe(row_t *curr){
  if(row_numOnes(*curr) == NUM_PROBES || MAX_COEFF == 0) return 0; // end as we covered all probes.
  row_t next = row_add(*curr, 1ll << (NUM_OUTS * D), 0); // just increment the probe combination
  if(row_numOnes(next) <= MAX_COEFF){
    *curr = next;
    return 1;
  }

  int i = 0;
  for(; i < ROW_VALUES_SIZE && curr->values[i] == 0; i++); // skip to the first 1
  *curr = row_add(*curr, 1ll << TAIL_1(curr->values[i]), i);

  for(i = ROW_VALUES_SIZE-1; i >= 0 && curr->values[i] == 0; i--); // skip highest 0s
  if( i * 64 + LEAD_1(curr->values[i])  >= NUM_TOT_OUTS) return 0; // if overflows

  return 1;

}

bool row_tryNextProbeAndOut(row_t *curr){
  if(row_tryNextOut(curr)) return 1;
  return row_tryNextProbe(curr);
}


// used as an iterator to get the next sub probe combination.
bool row_tryGetNext(row_t highestRow, row_t *curr){ // first is 0, return 0 for end
  if(row_eq(*curr, highestRow)) return 0;

  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    row_value_t curr_zeros = highestRow.values[i] ^ curr->values[i];  // curr_zeros has a 1 where curr has a meaningful 0
    if(curr_zeros == 0){
      curr->values[i] = 0;
      continue; // carry
    }

    row_value_t lowest_0 = 1ll << TAIL_1(curr_zeros); // lowest meningful 0 of curr
    curr->values[i] &= ~(lowest_0-1); // remove anything below the lowest 0
    curr->values[i] |= lowest_0; // change the lowest 0 to 1.
    break;
  }
  return 1;
}

