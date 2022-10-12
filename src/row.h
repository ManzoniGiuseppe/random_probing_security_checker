#ifndef _ROW_H_
#define _ROW_H_


#include "types.h"



#if NUM_TOT_OUTS <= 64
  #if NUM_TOT_OUTS <= 16
    #define ROW_BITS 16
    typedef uint16_t row_t;
  #elif NUM_TOT_OUTS <= 32
    #define ROW_BITS 32
    typedef uint32_t row_t;
  #else // NUM_TOT_OUTS <= 64
    #define ROW_BITS 64
    typedef uint64_t row_t;
  #endif

  __attribute__ ((const)) static inline row_t row_zero(void){  return 0;  }
  __attribute__ ((const)) static inline bool row_hasInput(row_t r, shift_t input){  return (r >> input) & 1;  }
  __attribute__ ((const)) static inline bool row_eq(row_t r0, row_t r1){  return r0 == r1;  }
  __attribute__ ((const)) static inline row_t row_or(row_t r0, row_t r1){  return r0 | r1;  }
  __attribute__ ((const)) static inline row_t row_and(row_t r0, row_t r1){  return r0 & r1;  }
  __attribute__ ((const)) static inline row_t row_xor(row_t r0, row_t r1){  return r0 ^ r1;  }
  __attribute__ ((const)) static inline row_t row_not(row_t r){  return ~r;  }
  __attribute__ ((const)) static inline shift_t row_count1(row_t r){  return COUNT_1(r);  }
  __attribute__ ((const)) static inline shift_t row_tail1(row_t r){  return TAIL_1(r);  }
  __attribute__ ((const)) static inline hash_s_t row_hash(row_t r, hash_s_t counter, shift_t bitSize){
    uint64_t hash = r + counter;
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);

    hash_s_t ret = 0;
    for(int i = 0; i < 64 ; i+=bitSize)
      ret ^= (hash >> i) & MASK_OF(bitSize);
    return ret;
  }

  //internal
  #define ROW_I_SET(row, index)  (row) |= 1ll << (index);
  #define ROW_I_RESET(row, index)  (row) &= ~(1ll << (index));
#else
  #define ROW_BITS -1

  #include "bitArray.h"

  typedef struct {
    BITARRAY_DEF_VAR(NUM_TOT_OUTS, values)
  } row_t;

  __attribute__ ((const)) static inline row_t row_zero(void){
    row_t ret;
    bitArray_zero(NUM_TOT_OUTS, ret.values);
    return ret;
  }

  __attribute__ ((const)) static inline row_t row_or(row_t r0, row_t r1){
    row_t ret;
    bitArray_or(NUM_TOT_OUTS, r0.values, r1.values, ret.values);
    return ret;
  }

  __attribute__ ((const)) static inline row_t row_and(row_t r0, row_t r1){
    row_t ret;
    bitArray_and(NUM_TOT_OUTS, r0.values, r1.values, ret.values);
    return ret;
  }

  __attribute__ ((const)) static inline row_t row_xor(row_t r0, row_t r1){
    row_t ret;
    bitArray_xor(NUM_TOT_OUTS, r0.values, r1.values, ret.values);
    return ret;
  }

  __attribute__ ((const)) static inline row_t row_not(row_t r){
    row_t ret;
    bitArray_not(NUM_TOT_OUTS, r.values, ret.values);
    return ret;
  }

  __attribute__ ((const)) static inline bool row_hasInput(row_t r, shift_t input){  return bitArray_get(r.values, input);  }
  __attribute__ ((const)) static inline bool row_eq(row_t r0, row_t r1){  return bitArray_eq(NUM_TOT_OUTS, r0.values, r1.values);  }
  __attribute__ ((const)) static inline shift_t row_count1(row_t r){  return bitArray_count1(NUM_TOT_OUTS, r.values);  }
  __attribute__ ((const)) static inline shift_t row_tail1(row_t r){  return bitArray_tail1(NUM_TOT_OUTS, r.values);  }
  __attribute__ ((const)) static inline hash_s_t row_hash(row_t r, hash_s_t counter, shift_t bitSize){  return bitArray_hash(NUM_TOT_OUTS, r.values, counter, bitSize);  }

  //internal
  #define ROW_I_SET(row, index)  bitArray_set((row).values, index);
  #define ROW_I_RESET(row, index)  bitArray_reset((row).values, index);
#endif



__attribute__ ((const)) static inline row_t row_singleInput(shift_t input){
  row_t ret = row_zero();
  ROW_I_SET(ret, input)
  return ret;
}


__attribute__ ((const)) shift_t row_maxShares(row_t r);
__attribute__ ((const)) row_t row_first(void);
bool row_tryNextOut(row_t *curr);
bool row_tryNextProbe(row_t *curr);
bool row_tryNextProbeAndOut(row_t *curr);
bool row_tryGetNext(row_t highestRow, row_t *curr);


#define ROW_ITERATE_OVER_ONES(row, I, CODE) {\
  row_t row_hfuek__row = (row);\
  while(!row_eq(row_hfuek__row, row_zero())){\
    shift_t I = row_tail1(row_hfuek__row);\
    ROW_I_RESET(row_hfuek__row, I)\
    { CODE }\
  }\
}

#define ITERATE_OVER_DIRECT_SUB_ROWS(row, SUB, CODE) {\
  row_t SUB = (row);/*duplicate*/\
  ROW_ITERATE_OVER_ONES(SUB, row_hfuek__i, {\
    ROW_I_RESET(SUB, row_hfuek__i)\
    { CODE }\
    ROW_I_SET(SUB, row_hfuek__i)\
  })\
}




#endif // _ROW_H_

