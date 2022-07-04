#include <stdio.h>
#include <stdlib.h>

#include "bdd.h"

#define FAIL(...)  { fprintf(stderr, __VA_ARGS__); exit(1); }

void bdd_dbg_print(bdd_t val);

int main(){
  bdd_t f = bdd_val_const(0);
  bdd_t t = bdd_val_const(1);
  if(f != bdd_val_const(0)) FAIL("false should have same reference\n")
  if(t == f) FAIL("t == f\n")
  if(bdd_op_not(f) != t) FAIL("!f != t\n")
  if(bdd_op_not(bdd_op_not(f)) != f) FAIL("!!f != f\n")
  if(bdd_op_xor(f,f) != f) FAIL("f^f != f\n")
  if(bdd_op_xor(f,t) != t) FAIL("f^t != t\n")
  if(bdd_op_xor(t,f) != t) FAIL("t^f != t\n")
  if(bdd_op_xor(t,t) != f) FAIL("t^t != f\n")
  if(bdd_op_and(f,f) != f) FAIL("f^f != f\n")
  if(bdd_op_and(f,t) != f) FAIL("f^t != f\n")
  if(bdd_op_and(t,f) != f) FAIL("t^f != f\n")
  if(bdd_op_and(t,t) != t) FAIL("t^t != t\n")

  fixed_cell_t one = 1ll << NUM_TOT_INS;
  {
    fixed_cell_t expected[NUM_NORND_COLS] = {one};
    fixed_cell_t ret[NUM_NORND_COLS] = {0};
    bdd_get_transformWithoutRnd(f, ret);
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(ret[i] != expected[i])
        FAIL("transform of f, at %d. expected=%d, ret=%d\n", i, expected[i], ret[i])
  }

  {
    fixed_cell_t expected[NUM_NORND_COLS] = {-one};
    fixed_cell_t ret[NUM_NORND_COLS] = {0};
    bdd_get_transformWithoutRnd(t, ret);
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(ret[i] != expected[i])
        FAIL("transform of t, at %d. expected=%d, ret=%d\n", i, expected[i], ret[i])
  }

  bdd_t b3 = bdd_val_single(3);
  if(b3 != bdd_val_single(3)) FAIL("b3 should have same reference\n")
  if(b3 != bdd_val_single(3)) FAIL("b3 should have same reference\n")

  if(b3 != bdd_op_and(b3,t)) FAIL("b3 & t\n")
  if(f != bdd_op_and(b3,f)) FAIL("b3 & f\n")
  if(f != bdd_op_and(b3,bdd_op_not(b3))) FAIL("b3 & !b3\n")

  {
    fixed_cell_t expected[NUM_NORND_COLS] = {0};
    expected[8] = one;
    fixed_cell_t ret[NUM_NORND_COLS] = {0};
    bdd_get_transformWithoutRnd(b3, ret);
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(ret[i] != expected[i])
        FAIL("transform of b3, at %d. expected=%d, ret=%d\n", i, expected[i], ret[i])
  }

  bdd_t b0 = bdd_val_single(0);
  {
    fixed_cell_t expected[NUM_NORND_COLS] = {0};
    expected[1] = one;
    fixed_cell_t ret[NUM_NORND_COLS] = {0};
    bdd_get_transformWithoutRnd(b0, ret);
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(ret[i] != expected[i])
        FAIL("transform of b0, at %d. expected=%d, ret=%d\n", i, expected[i], ret[i])
  }

  bdd_t b1 = bdd_val_single(1);
  bdd_t b2 = bdd_val_single(2);
  if(b1 == b2) FAIL("b1 == b2\n")
  if(b1 == b3) FAIL("b1 == b3\n")
  if(bdd_op_and(b2,b3) != bdd_op_and(b2,b3)) FAIL("b2 & b3 = b2 & b3\n")
  if(bdd_op_and(bdd_op_and(b1,b2),b3) != bdd_op_and(b1,bdd_op_and(b2,b3))) FAIL("b1 & (b2 & b3) = (b1 & b2) & b3\n")
  if(bdd_op_xor(bdd_op_xor(b1,b2),b3) != bdd_op_xor(b1,bdd_op_xor(b2,b3))) FAIL("b1 ^ (b2 ^ b3) = (b1 ^ b2) ^ b3\n")

  {
    fixed_cell_t expected[NUM_NORND_COLS] = {0};
    expected[6] = one;
    fixed_cell_t ret[NUM_NORND_COLS] = {0};
    bdd_get_transformWithoutRnd(bdd_op_xor(b1,b2), ret);
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(ret[i] != expected[i])
        FAIL("transform of b1 ^ b2, at %d. expected=%d, ret=%d\n", i, expected[i], ret[i])
  }

  {
    fixed_cell_t expected[NUM_NORND_COLS] = {0};
    expected[0] = one/2;
    expected[2] = one/2;
    expected[4] = one/2;
    expected[6] = -one/2;
    fixed_cell_t ret[NUM_NORND_COLS] = {0};
    bdd_get_transformWithoutRnd(bdd_op_and(b1,b2), ret);
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(ret[i] != expected[i])
        FAIL("transform of b1 & b2, at %d. expected=%d, ret=%d\n", i, expected[i], ret[i])
  }

  {
    fixed_cell_t expected[NUM_NORND_COLS] = {0};
    expected[14] = one;
    fixed_cell_t ret[NUM_NORND_COLS] = {0};
    bdd_get_transformWithoutRnd(bdd_op_xor(bdd_op_xor(b1,b2),b3), ret);
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(ret[i] != expected[i])
        FAIL("transform of b1 ^ (b2 ^ b3), at %d. expected=%d, ret=%d\n", i, expected[i], ret[i])
  }

  {
    fixed_cell_t expected[NUM_NORND_COLS] = {0};
    expected[1] = one/2;
    expected[3] = one/2;
    expected[5] = one/2;
    expected[7] = -one/2;
    fixed_cell_t ret[NUM_NORND_COLS] = {0};
    bdd_get_transformWithoutRnd(bdd_op_xor(b0, bdd_op_and(b1,b2)), ret);
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(ret[i] != expected[i])
        FAIL("transform of b0 ^ (b1 & b2), at %d. expected=%d, ret=%d\n", i, expected[i], ret[i])
  }

  if(bdd_op_and(bdd_op_xor(b2,b3),b1) != bdd_op_xor(bdd_op_and(b2,b1),bdd_op_and(b1,b3))) FAIL("b1 & (b2 ^ b3) = (b1 & b2) ^ (b1 & b3)\n")

  printf("Success!\n");
}
