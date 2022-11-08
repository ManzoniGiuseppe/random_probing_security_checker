//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _BITARRAY_H_
#define _BITARRAY_H_


#include "types.h"
#include "string.h"
#include "assert.h"



#define DBG_FILE "bitArray"
#define DBG_LVL DBG_BITARRAY




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

T__THREAD_SAFE static inline bool bitArray_get(shift_t bits, bitArray_t arr, shift_t index){
  ON_DBG(DBG_LVL_TOFIX, {
    if(index >= bits) FAIL("bitArray: Asking for bit %d of an array with size %d\n", index, bits)
  })
  return (arr[BITARRAY_I_VAL_INDEX(index)] >> BITARRAY_I_BIT_INDEX(index)) & 1;
}
T__THREAD_SAFE static inline bool bitArray_eq(shift_t bits, bitArray_t arr1, bitArray_t arr2){
  return memcmp(arr1, arr2, BITARRAY_I_BYTE_SIZE(bits)) == 0;
}

#define BITARRAY_DEF_VAR(bits, VAR)   bitArray_i_value_t VAR[BITARRAY_I_SIZE(bits)];
#define BITARRAY_CALLOC(bits, what)   mem_calloc(sizeof(bitArray_i_value_t), BITARRAY_I_SIZE(bits), what)

// it's assured that allocating the memory with calloc will result to an array of zeros.
static inline void bitArray_zero(shift_t bits, bitArray_t arr){
  memset(arr, 0, BITARRAY_I_BYTE_SIZE(bits));
}
static inline void bitArray_set(shift_t bits, bitArray_t arr, shift_t index){
  ON_DBG(DBG_LVL_TOFIX, {
    if(index >= bits) FAIL("bitArray: Asking to set bit %d of an array with size %d\n", index, bits)
  })
  arr[BITARRAY_I_VAL_INDEX(index)] |= 1ll << BITARRAY_I_BIT_INDEX(index);
}
static inline void bitArray_reset(shift_t bits, bitArray_t arr, shift_t index){
  ON_DBG(DBG_LVL_TOFIX, {
    if(index >= bits) FAIL("bitArray: Asking to reset bit %d of an array with size %d\n", index, bits)
  })
  arr[BITARRAY_I_VAL_INDEX(index)] &= ~( 1ll << BITARRAY_I_BIT_INDEX(index) );
}

T__THREAD_SAFE static inline void bitArray_or(shift_t bits, bitArray_t arr1, bitArray_t arr2, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = arr1[i] | arr2[i];
}
T__THREAD_SAFE static inline void bitArray_and(shift_t bits, bitArray_t arr1, bitArray_t arr2, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = arr1[i] & arr2[i];
}
T__THREAD_SAFE static inline void bitArray_xor(shift_t bits, bitArray_t arr1, bitArray_t arr2, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = arr1[i] ^ arr2[i];
}
T__THREAD_SAFE static inline void bitArray_not(shift_t bits, bitArray_t arr, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = ~arr[i];
  if(BITARRAY_I_BIT_INDEX(bits) != 0)
    ret[BITARRAY_I_SIZE(bits)-1] &= MASK_OF(BITARRAY_I_BIT_INDEX(bits));
}
T__THREAD_SAFE static inline void bitArray_copy(shift_t bits, bitArray_t arr, bitArray_t ret){
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret[i] = arr[i];
}

T__THREAD_SAFE static inline shift_t bitArray_count1(shift_t bits, bitArray_t arr){
  shift_t ret = 0;
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret += COUNT_1(arr[i]);
  return ret;
}
T__THREAD_SAFE static inline shift_t bitArray_tail1(shift_t bits, bitArray_t arr){
  shift_t i = 0;
  for(; i < BITARRAY_I_SIZE(bits) && arr[i] == 0; i++); /*find the lowest non zero, if any*/
  assert(i != BITARRAY_I_SIZE(bits)); /*must have something*/
  return TAIL_1(arr[i]) + i * BITARRAY_I_VALUE_BITS;
}
T__THREAD_SAFE static inline shift_t bitArray_lead1(shift_t bits, bitArray_t arr){
  shift_t i = BITARRAY_I_SIZE(bits)-1;
  for(; i >= 0 && arr[i] == 0; i--); /*find the highest non zero, if any*/
  assert(i != -1); /*must have something*/
  return LEAD_1(arr[i]) + i * BITARRAY_I_VALUE_BITS;
}

T__THREAD_SAFE static inline shift_t bitArray_count1And(shift_t bits, bitArray_t arr1, bitArray_t arr2){
  shift_t ret = 0;
  for(shift_t i = 0; i < BITARRAY_I_SIZE(bits); i++)
    ret += COUNT_1(arr1[i] & arr2[i]);
  return ret;
}

static inline bool bitArray_localInc(shift_t bits, bitArray_t arr, shift_t fromBit){ // returns if it overflows
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

T__THREAD_SAFE static inline uint64_t bitArray_lowest_get(shift_t bits, bitArray_t arr, shift_t numBits){
  ON_DBG(DBG_LVL_TOFIX, {
    if(numBits > bits) FAIL("bitArray: Asking to get the lowest %d bits of an array with size %d\n", numBits, bits)
    if(numBits > BITARRAY_I_VALUE_BITS) FAIL("Asking to get %d bits, more than the word used to store the values", numBits);
  })
  return arr[0] & MASK_OF(numBits);
}

static inline void bitArray_lowest_set(shift_t bits, bitArray_t arr, shift_t numBits, uint64_t value){
  ON_DBG(DBG_LVL_TOFIX, {
    if(numBits > bits) FAIL("bitArray: Asking to set the lowest %d bits of an array with size %d\n", numBits, bits)
    if(numBits > BITARRAY_I_VALUE_BITS) FAIL("Asking to set %d bits, more than the word used to store the values", numBits);
  })
  arr[0] &= ~MASK_OF(numBits);
  arr[0] |= value;
}

static inline void bitArray_lowest_reset(shift_t bits, bitArray_t arr, shift_t numBits){
  ON_DBG(DBG_LVL_TOFIX, {
    if(numBits > bits) FAIL("bitArray: Asking to reset the lowest %d bits of an array with size %d\n", numBits, bits)
  })
  int i;
  for(i = 0; i < BITARRAY_I_VAL_INDEX(numBits); i++){
    arr[i] = 0;
  }
  arr[i] &= ~MASK_OF(BITARRAY_I_BIT_INDEX(numBits));
}


static inline void bitArray_toStr(shift_t bits, bitArray_t in, char *ret){ // ret[2+BITARRAY_I_BYTE_SIZE(bits)*2+1]
  shift_t size = 2 + BITARRAY_I_BYTE_SIZE(bits)*2 + 1;
  ret[size -1] = '\0';
  ret[0] = '0';
  ret[1] = 'x';

  for(int i = 0; i < size-3; i++){
    int val = 0;
    if(i*4 < bits) val |= bitArray_get(bits, in, i*4);
    if(i*4+1 < bits) val |= bitArray_get(bits, in, i*4+1) << 1;
    if(i*4+2 < bits) val |= bitArray_get(bits, in, i*4+2) << 2;
    if(i*4+3 < bits) val |= bitArray_get(bits, in, i*4+3) << 3;
    ret[size -2-i] = val <= 9 ? val + '0' : val - 10 + 'A';
  }
}



#undef DBG_FILE
#undef DBG_LVL


#endif // _BITARRAY_H_

