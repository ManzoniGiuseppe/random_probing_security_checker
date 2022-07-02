#include <stdio.h>
#include <stdlib.h>

#include "bdd.h"

#define FAIL(...)  { fprintf(stderr, __VA_ARGS__); exit(1); }



int main(){
  bdd_t f = bdd_val_const(0);
  bdd_t t = bdd_val_const(1);
  if(f != bdd_val_const(0)) FAIL("false should have same reference\n")
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

  bdd_t b3 = bdd_val_single(3);
  if(b3 != bdd_val_single(3)) FAIL("b3 should have same reference\n")
  if(b3 != bdd_val_single(3)) FAIL("b3 should have same reference\n")

  printf("Success!\n");
}
