//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _NUMBER_H_
#define _NUMBER_H_

#include <gmp.h>



// -- intenral defs

#define NUMBER_I_VALUE_BITS  (GMP_NUMB_BITS)
#define NUMBER_I_BYTE_SIZE(totBits)   ( NUMBER_SIZE(totBits) * sizeof(number_t) )
#define NUMBER_I_VAL_INDEX(index)   ( (index) / NUMBER_I_VALUE_BITS )
#define NUMBER_I_BIT_INDEX(index)   ( (index) % NUMBER_I_VALUE_BITS )


// -- public

typedef mp_limb_t number_t;
#define NUMBER_SIZE(totBits)   ( ((totBits) + NUMBER_I_VALUE_BITS - 1) / NUMBER_I_VALUE_BITS )



T__THREAD_SAFE static inline void number_add(size_t size, const number_t *v0, const number_t *v1, number_t *ret){
  mpn_add_n(ret, v0, v1, size);
}

T__THREAD_SAFE static inline void number_sub(size_t size, const number_t *v0, const number_t *v1, number_t *ret){
  mpn_sub_n(ret, v0, v1, size);
}

T__THREAD_SAFE static inline bool number_gez(size_t size, const number_t *v){
  return ((v[size-1] >> (NUMBER_I_VALUE_BITS-1)) & 1) == 0;
}

T__THREAD_SAFE static inline void number_copy(size_t size, const number_t *v, number_t *ret){
  mpn_copyi(ret, v, size);
}

T__THREAD_SAFE static inline bool number_isZero(size_t size, const number_t *v){
  return mpn_zero_p(v, size);
}

T__THREAD_SAFE static inline void number_zero(size_t size, number_t *ret){
  mpn_zero(ret, size);
}

T__THREAD_SAFE static inline void number_one(size_t size, size_t fracBits, number_t *ret){
  mpn_zero(ret, size);
  ret[NUMBER_I_VAL_INDEX(fracBits)] = ((number_t)1) << NUMBER_I_BIT_INDEX(fracBits);
}

T__THREAD_SAFE static inline bool number_eq(size_t size, const number_t *v1, const number_t *v2){
  return mpn_cmp(v1, v2, size) == 0;
}

static inline void number_local_negation(size_t size, number_t *v){
  mpn_neg(v, v, size);
}

static inline void number_local_div2(size_t size, number_t *v){
  bool gez = number_gez(size, v);
  mpn_rshift(v, v, size, 1);
  v[size-1] |= (((number_t)gez) << (NUMBER_I_VALUE_BITS-1));
}

static inline void number_local_lshift(size_t size, number_t *v, shift_t ofBits){
  ssize_t startingIndex = NUMBER_I_VAL_INDEX(ofBits);
  ofBits = NUMBER_I_BIT_INDEX(ofBits);

  {
    ssize_t c = size-1;
    for(; c >= startingIndex; c--){
      v[c] = v[c-startingIndex];
    }
    for(; c >= 0; c--){
      v[c] = 0;
    }
  }

  if(ofBits == 0) return;

  mpn_lshift(v+startingIndex, v+startingIndex, size-startingIndex, ofBits);
}





#endif // _NUMBER_H_

