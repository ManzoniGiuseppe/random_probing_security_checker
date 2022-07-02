#include "probeComb.h"
#include "row.h"
#include "gadget.h"


row_t probeComb_getHighestRow(probeComb_t curr_comb, row_t output){
  row_t ret = output;
  for(shift_t i = 0; i < NUM_PROBES; i++){
    if(curr_comb[i] > 0){
      ret = row_or(ret, row_singleInput(i + NUM_OUTS * D));
    }
  }
  return ret;
}


// return the logaritm, how much it needs to be shifted
shift_t probeComb_getRowMulteplicity(probeComb_t curr_comb){
  // comb: 1,3,4,0,1
  // row:  0,0,0,0,0

  // 0 0 0 0 0 *(4 0)  = 1
  // 0 2 0 0 0 *(3 2)  = 3
  // 0 0 2 0 0 *(4 2)  = 6
  // 0 0 4 0 0 *(4 4)  = 1
  // 0 2 2 0 0 *(3 2)(4 2) = 3*6 = 18
  // 0 2 4 0 0 *(3 2) = 3
  //             tot = 11+18+3 = 32
  // 1*4*8*1*1 = 32


  long ret=0;
  for(shift_t i = 0; i < NUM_PROBES; i++){
    if(curr_comb[i] > 1){
      ret += curr_comb[i]-1;  // sum of even/odd binomial (n k) is 2^(n-1)
    }
  }

  return ret;
}

bool probeComb_tryIncrement(probeComb_t curr_comb, shift_t* curr_count){ // inited respectively to all 0s and 0.
  shift_t to_inc = 0;
  if(*curr_count < MAX_COEFF) to_inc = 0; // if there are probes left, increment normally
  else { // already hit the maximum.
    shift_t i;
    for(i = 0; i < NUM_PROBES && curr_comb[i] == 0; i++);  // find lowest position != 0

    *curr_count -= curr_comb[i];  // consider is as if it has hit its maximum and increment it
    curr_comb[i] = 0;
    to_inc = i+1; // ask to increment the next, due to overflow
  }

  bool carry = 1;
  for(shift_t i = to_inc; i < NUM_PROBES && carry; i++){
    if(curr_comb[i] < gadget_probeMulteplicity[i]){ // if can still increment without overflowing
      curr_comb[i] += 1;
      *curr_count += 1;
      carry = 0;
    }else{ // reached maximum
      *curr_count -= curr_comb[i];
      curr_comb[i] = 0;
    }
  }

  return !carry;  // if there is still a carry, we looped back. Otherwise we succeded.
}


static double binomial(int n, int k){
  if(k > n-k) return binomial(n, n-k);

  double ret = 1.0;
  for(int i = n-k+1; i <= n; i++)
    ret *= i;
  for(int i = 2; i <= k; i++)
    ret /= i;

  return ret;
}

double probeComb_getProbesMulteplicity(probeComb_t curr_comb){
  double ret = 1.0;
  for(shift_t i = 0; i < NUM_PROBES; i++)
    ret *= binomial(gadget_probeMulteplicity[i], curr_comb[i]);
  return ret;
}

