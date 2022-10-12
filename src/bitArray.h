#ifndef _BITARRAY_H_
#define _BITARRAY_H_


#include "types.h"
#include "string.h"
#include "assert.h"



// -- intenral defs


#ifndef BITARRAY_I_VALUE_LOG_BITS // for testing
  typedef uint64_t bitArray_i_value_t;
  #define BITARRAY_I_VALUE_LOG_BITS 6  // 64 bit
#endif

#define BITARRAY_I_VALUE_BITS  (1ll << BITARRAY_I_VALUE_LOG_BITS)
#define BITARRAY_I_SIZE(bits)   ( ((bits) + BITARRAY_I_VALUE_BITS - 1) / BITARRAY_I_VALUE_BITS )
#define BITARRAY_I_BYTE_SIZE(bits)   ( BITARRAY_I_SIZE(bits) * sizeof(bitArray_i_value_t) )
#define BITARRAY_I_VAL_INDEX(index)   ( (index) >> BITARRAY_I_VALUE_LOG_BITS )
#define BITARRAY_I_BIT_INDEX(index)   ( (index) & MASK_OF(BITARRAY_I_VALUE_LOG_BITS) )


// -- public


typedef bitArray_i_value_t *bitArray_t;

static inline bool bitArray_get(bitArray_t arr, shift_t index){
  return (arr[BITARRAY_I_VAL_INDEX(index)] >> BITARRAY_I_BIT_INDEX(index)) & 1;
}
static inline bool bitArray_eq(shift_t bits, bitArray_t arr1, bitArray_t arr2){
  return memcmp(arr1, arr2, BITARRAY_I_BYTE_SIZE(bits)) == 0;
}

#define BITARRAY_DEF_VAR(bits, VAR)   bitArray_i_value_t VAR[BITARRAY_I_SIZE(bits)];

static inline void bitArray_zero(shift_t bits, bitArray_t arr){
  memset(arr, 0, BITARRAY_I_BYTE_SIZE(bits));
}
static inline void bitArray_set(bitArray_t arr, shift_t index){
  arr[BITARRAY_I_VAL_INDEX(index)] |= 1ll << BITARRAY_I_BIT_INDEX(index);
}
static inline void bitArray_reset(bitArray_t arr, shift_t index){
  arr[BITARRAY_I_VAL_INDEX(index)] &= ~( 1ll << BITARRAY_I_BIT_INDEX(index) );
}

static inline void bitArray_or(shift_t bits, bitArray_t arr1, bitArray_t arr2, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = arr1[i] | arr2[i];
}
static inline void bitArray_and(shift_t bits, bitArray_t arr1, bitArray_t arr2, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = arr1[i] & arr2[i];
}
static inline void bitArray_xor(shift_t bits, bitArray_t arr1, bitArray_t arr2, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = arr1[i] ^ arr2[i];
}
static inline void bitArray_not(shift_t bits, bitArray_t arr, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = ~arr[i];
  if(BITARRAY_I_BIT_INDEX(bits) != 0)
    ret[BITARRAY_I_SIZE(bits)-1] &= MASK_OF(BITARRAY_I_BIT_INDEX(bits));
}

static inline shift_t bitArray_count1(shift_t bits, bitArray_t arr){
  shift_t ret = 0;
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret += COUNT_1(arr[i]);
  return ret;
}
static inline shift_t bitArray_tail1(shift_t bits, bitArray_t arr){
  shift_t i = 0;
  for(; i < BITARRAY_I_SIZE(bits) && arr[i] == 0; i++); /*find the lowest non zero, if any*/
  assert(i != BITARRAY_I_SIZE(bits)); /*must have something*/
  return TAIL_1(arr[i]) + i * BITARRAY_I_VALUE_BITS;
}
static inline shift_t bitArray_lead1(shift_t bits, bitArray_t arr){
  shift_t i = BITARRAY_I_SIZE(bits)-1;
  for(; i >= 0 && arr[i] == 0; i--); /*find the highest non zero, if any*/
  assert(i != -1); /*must have something*/
  return LEAD_1(arr[i]) + i * BITARRAY_I_VALUE_BITS;
}

static inline bool bitArray_localInc(shift_t bits, bitArray_t arr, shift_t fromBit){
  bitArray_i_value_t toAdd = 1ll << BITARRAY_I_BIT_INDEX(fromBit);
  int i = BITARRAY_I_VAL_INDEX(fromBit);
  for(; toAdd != 0 && i < BITARRAY_I_SIZE(bits); i++){
    arr[i] += toAdd;
    toAdd = arr[i] < toAdd; /*overflow*/
  }

  if(BITARRAY_I_BIT_INDEX(bits) == 0) /*highest value fully used by the array*/
    return toAdd;
  if(i != BITARRAY_I_SIZE(bits)) /*current highest value == 0*/
    return 0;
  return  arr[BITARRAY_I_SIZE(bits)-1] >= MASK_OF(BITARRAY_I_BIT_INDEX(bits));
}

static inline hash_s_t bitArray_hash(shift_t bits, bitArray_t arr, hash_s_t counter, shift_t hashSize){
  uint64_t hash = 0;

  for(int i = 0; i < BITARRAY_I_SIZE(bits); i++){
    uint64_t it = arr[i] + counter;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  }

  hash_s_t ret = 0;
  for(int i = 0; i < 64 ; i+=hashSize)
    ret ^= (hash >> i) & MASK_OF(hashSize);
  return ret;
}


static inline uint64_t bitArray_lowest_get(bitArray_t arr, shift_t numBits){
  assert(numBits <= BITARRAY_I_VALUE_BITS);
  return arr[0] & MASK_OF(numBits);
}

static inline void bitArray_lowest_set(bitArray_t arr, shift_t numBits, uint64_t value){
  assert(numBits <= BITARRAY_I_VALUE_BITS);
  arr[0] &= ~MASK_OF(numBits);
  arr[0] |= value;
}


#endif // _BITARRAY_H_

