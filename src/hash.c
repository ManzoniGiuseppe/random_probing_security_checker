//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "hash.h"


#define DBG_FILE  "hash"
#define DBG_LVL DBG_HASH



T__THREAD_SAFE hash_t hash_calculate(size_t size, void *value, unsigned int counter){
  uint_fast64_t v0 = 0x736f6d6570736575 + 0x7C94A7 * (counter + 0xC1);
  uint_fast64_t v1 = 0x646f72616e646f6d;

  size_t i = 0;
  for(; i+sizeof(uint64_t) <= size; i+=sizeof(uint64_t)){
    uint_fast64_t it = ((uint64_t*)value)[i/sizeof(uint64_t)];

    v0 ^= it;

    it = ((it * 11) ^ ROT(it, 17)) + i + counter;
    it = (it * 11) ^ ROT(it, 17);
    it = (it * 11) ^ ROT(it, 17);

    v0 = (v0 * 13) + ROT(it, 23);
    v0 = (v0 * 13) + ROT(v0, 23);

    v1 = (v1 * 19) + ROT(v0, 29);
    v1 = (v1 * 19) + ROT(v1, 29);
  }
  for(; i < size; i++){
    uint_fast64_t it = ((uint8_t*)value)[i/sizeof(uint8_t)];

    v0 ^= it;

    it = ((it * 11) ^ ROT(it, 17)) + i + counter;
    it = (it * 11) ^ ROT(it, 17);
    it = (it * 11) ^ ROT(it, 17);

    v0 = (v0 * 13) + ROT(it, 23);
    v0 = (v0 * 13) + ROT(v0, 23);

    v1 = (v1 * 19) + ROT(v0, 29);
    v1 = (v1 * 19) + ROT(v1, 29);
  }


  uint_fast32_t ret = 0;
  for(int i = 0; i < 64 ; i+=HASH_WIDTH)
    ret ^= (v1 >> i) & MASK_OF(HASH_WIDTH);
  return (hash_t){ ret };
}
