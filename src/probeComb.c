//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>

#include "probeComb.h"
#include "row.h"
#include "gadget.h"


void probeComb_getHighestRow(probeComb_t curr_comb, gadget_t *g, bitArray_t ret){
  bitArray_zero(g->numTotOuts, ret);

  for(wire_t i = 0; i < g->numProbes; i++)
    if(curr_comb[i] > 0)
      bitArray_set(g->numTotOuts, ret, i + g->numOuts * g->d);
}


// return the logaritm, how much it needs to be shifted
shift_t probeComb_getRowMulteplicity(probeComb_t curr_comb, gadget_t *g){
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
  for(wire_t i = 0; i < g->numProbes; i++){
    if(curr_comb[i] > 1){
      ret += curr_comb[i]-1;  // sum of even/odd binomial (n k) is 2^(n-1)
    }
  }

  return ret;
}

bool probeComb_tryIncrement(probeComb_t curr_comb, wire_t* curr_count, wire_t maxCoeff, gadget_t *g){ // inited respectively to all 0s and 0.
  wire_t to_inc;
  if(*curr_count < maxCoeff) to_inc = 0; // if there are probes left, increment normally
  else { // already hit the maximum.
    wire_t i;
    for(i = 0; i < g->numProbes && curr_comb[i] == 0; i++);  // find lowest position != 0

    *curr_count -= curr_comb[i];  // consider is as if it has hit its maximum and increment it
    curr_comb[i] = 0;
    to_inc = i+1; // ask to increment the next, due to overflow
  }

  bool carry = 1;
  for(wire_t i = to_inc; i < g->numProbes && carry; i++){
    if(curr_comb[i] < g->probeMulteplicity[i]){ // if can still increment without overflowing
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


double probeComb_getProbesMulteplicity(probeComb_t curr_comb, gadget_t *g){
  double ret = 1.0;
  for(wire_t i = 0; i < g->numProbes; i++)
    ret *= binomial(g->probeMulteplicity[i], curr_comb[i]);
  return ret;
}


void probeComb_firstWhoseImageIs(bitArray_t highest_row, probeComb_t ret_comb, wire_t *ret_count, wire_t maxCoeff, gadget_t *g){
  *ret_count = 0;
  for(wire_t i = 0; i < g->numProbes; i++){
    *ret_count += ret_comb[i] = bitArray_get(g->numTotOuts, highest_row, i + g->numOuts * g->d);
  }
  if(*ret_count > maxCoeff) FAIL("probeComb: more probes than maxCoeff")
}

bool probeComb_nextWithSameImage(probeComb_t curr_comb, wire_t *curr_count, wire_t maxCoeff, gadget_t *g){
  wire_t to_inc;
  if(*curr_count < maxCoeff) to_inc = 0; // if there are probes left, increment normally
  else { // already hit the maximum.
    wire_t i;
    for(i = 0; i < g->numProbes && curr_comb[i] <= 1; i++);  // find lowest position that can be decremented
    if(i == g->numProbes) return 0; // can't move any bit to increment.

    *curr_count -= curr_comb[i]-1;  // consider is as if it has hit its maximum and increment it
    curr_comb[i] = 1;
    to_inc = i+1; // ask to increment the next, due to overflow
  }

  bool carry = 1;
  for(wire_t i = to_inc; i < g->numProbes && carry; i++){
    if(curr_comb[i] == 0){
      continue; // can't change the highest_row relative to the probe combination
    }else if(curr_comb[i] < g->probeMulteplicity[i]){ // if can still increment without overflowing
      curr_comb[i] += 1;
      *curr_count += 1;
      carry = 0;
    }else{ // reached maximum
      *curr_count -= curr_comb[i]-1;
      curr_comb[i] = 1;
    }
  }

  return !carry;  // if there is still a carry, we looped back. Otherwise we succeded.
}
