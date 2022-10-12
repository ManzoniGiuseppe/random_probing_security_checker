#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"
#include "row.h"




#if ROW_BITS > 0 // not bitarray
  #define ROW_GET_OUTPUTS(row)  ((row) & MASK_OF(D*NUM_OUTS))

  #define ROW_SET_OUTPUTS(row, outputs)  {\
    (row) &= ~MASK_OF(D*NUM_OUTS);\
    (row) |= (outputs);\
  }
  #define ROW_LOCAL_INC(row, fromBit, retHasOverflowed)  {\
    (row) += 1ll << (fromBit);\
    if(NUM_TOT_OUTS == ROW_BITS) /*the bit space is fully used by the array*/\
      (retHasOverflowed) = (row) < 1ll << (fromBit);\
    else\
      (retHasOverflowed) = (row) > MASK_OF(NUM_TOT_OUTS);\
  }
#else
  #define ROW_GET_OUTPUTS(row)   bitArray_lowest_get((row).values, D*NUM_OUTS)

  #define ROW_SET_OUTPUTS(row, outputs)  bitArray_lowest_set((row).values, D*NUM_OUTS, outputs);
  #define ROW_LOCAL_INC(row, fromBit, retHasOverflowed)   (retHasOverflowed) = bitArray_localInc(NUM_TOT_OUTS, (row).values, fromBit);
#endif



// -- getRow

shift_t row_maxShares(row_t r){
  shift_t max = 0;
  uint64_t outputs = ROW_GET_OUTPUTS(r);
  for(int o = 0; o < NUM_OUTS; o++){
    shift_t val = COUNT_1(outputs & (MASK_OF(D) << (o*D)));
    max = MAX(max, val);
  }
  return max;
}

row_t row_first(){
  return row_zero();
}

static bool row_tryNextOut__i(row_t *curr, shift_t o){
  uint64_t outputs = ROW_GET_OUTPUTS(*curr);

  uint64_t outMask = MASK_OF(D) << (o*D);
  uint64_t upperTOutMask = MASK_OF(T) << (o*D + D-T);

  if(COUNT_1(outputs & upperTOutMask) == T){ // all 1s are at the top -> no output conmbinations left
    outputs &= ~outMask; // reset this out
    ROW_SET_OUTPUTS(*curr, outputs)
    return 0;
  }

  shift_t fromBit;
  if(COUNT_1(outputs & outMask) < T){
    fromBit = o*D; // can't overwrite the higer bits as T < D
  }else{ // COUNT_1(out) == T
    fromBit = TAIL_1(outputs & outMask); // can't overwrite the higer bits due to the check with upperTOutMask
  }
  bool hasOverflowed __attribute__((unused));
  ROW_LOCAL_INC(*curr, fromBit, hasOverflowed);  // can't overwrite the higer bits as T < D
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
  if(ROW_GET_OUTPUTS(*curr) != 0) FAIL("row.c: BUG: row_tryNextProbe's parameter includes an output bit\n");

  if(row_count1(*curr) < MAX_COEFF){
    bool hasOverflowed;
    ROW_LOCAL_INC(*curr, NUM_OUTS * D, hasOverflowed)
    return !hasOverflowed;
  }

  if(MAX_COEFF == 0) return 0; // only one probe combination: all zeros

  int lowest1 = row_tail1(*curr);
  bool hasOverflowed;
  ROW_LOCAL_INC(*curr, lowest1, hasOverflowed)
  return !hasOverflowed;
}

bool row_tryNextProbeAndOut(row_t *curr){
  if(row_tryNextOut(curr)) return 1;
  return row_tryNextProbe(curr);
}


// used as an iterator to get the next sub probe combination.
bool row_tryGetNext(row_t highestRow, row_t *curr){ // first is 0, return 0 for end
  if(row_eq(*curr, highestRow)) return 0;

  row_t curr_zeros = row_xor(highestRow, *curr); // curr_zeros has a 1 where curr has a meaningful 0
  shift_t lowest0 = row_tail1(curr_zeros); // incementing of 1 = setting the lowest 0 to 1 and all the lower 1s to 0.

  ROW_I_SET(*curr, lowest0) // do it here to ensure there is always at least one 1.
  shift_t lowest1;
  while((lowest1 = row_tail1(*curr)) < lowest0)
    ROW_I_RESET(*curr, lowest1)

  return 1;
}

