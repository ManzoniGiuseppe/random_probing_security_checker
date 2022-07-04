#include <stdio.h>
#include <stdlib.h>

#include "bdd.h"

static void print_transform(bdd_t val){
  fixed_cell_t ret[NUM_NORND_COLS] = {0};
  bdd_get_transformWithoutRnd(val, ret);
  for(int i = 0; i < NUM_NORND_COLS; i++)
    printf("%d, ", ret[i]);
  printf("\n");
}


int main(){
  bdd_t f = bdd_val_const(0);
  bdd_t t = bdd_val_const(1);
  print_transform(f);
  print_transform(t);

  bdd_t b0 = bdd_val_single(0);
  bdd_t b1 = bdd_val_single(1);
  bdd_t b2 = bdd_val_single(2);
  bdd_t b3 = bdd_val_single(3);
  print_transform(b0);
  print_transform(b1);
  print_transform(bdd_op_and(bdd_op_and(b1,b2),b3));
  print_transform(bdd_op_xor(bdd_op_xor(b1,b2),b3));
  print_transform(bdd_op_and(bdd_op_xor(b1,b2),b3));
  print_transform(bdd_op_xor(bdd_op_and(b1,b2),b3));
  print_transform(bdd_op_xor(bdd_op_and(b2,b1),bdd_op_and(b1,b3)));
  print_transform(bdd_op_and(bdd_op_xor(b2,b1),bdd_op_xor(b1,b3)));
  print_transform(bdd_op_xor(bdd_op_and(b0,b1),bdd_op_and(b2,b3)));


  bdd_t r0 = bdd_val_single(6);
  bdd_t r1 = bdd_val_single(7);
  bdd_t r2 = bdd_val_single(8);
  bdd_t r3 = bdd_val_single(9);

  print_transform(r0);
  print_transform(r1);

  print_transform(bdd_op_and(r1,b2));
  print_transform(bdd_op_and(r1,r2));
  print_transform(bdd_op_xor(r1,b2));
  print_transform(bdd_op_xor(r1,r2));

  print_transform(bdd_op_and(bdd_op_and(r1,r2),r3));
  print_transform(bdd_op_xor(bdd_op_xor(r1,r2),r3));
  print_transform(bdd_op_and(bdd_op_xor(r1,r2),r3));
  print_transform(bdd_op_xor(bdd_op_and(r1,r2),r3));

  print_transform(bdd_op_and(bdd_op_and(r1,r2),b3));
  print_transform(bdd_op_xor(bdd_op_xor(r1,r2),b3));
  print_transform(bdd_op_and(bdd_op_xor(r1,r2),b3));
  print_transform(bdd_op_xor(bdd_op_and(r1,r2),b3));

  print_transform(bdd_op_and(bdd_op_and(r1,b2),r3));
  print_transform(bdd_op_xor(bdd_op_xor(r1,b2),r3));
  print_transform(bdd_op_and(bdd_op_xor(r1,b2),r3));
  print_transform(bdd_op_xor(bdd_op_and(r1,b2),r3));

  print_transform(bdd_op_and(bdd_op_and(r1,b2),b3));
  print_transform(bdd_op_xor(bdd_op_xor(r1,b2),b3));
  print_transform(bdd_op_and(bdd_op_xor(r1,b2),b3));
  print_transform(bdd_op_xor(bdd_op_and(r1,b2),b3));

  print_transform(bdd_op_xor(bdd_op_and(r2,r1),bdd_op_and(r1,r3)));
  print_transform(bdd_op_xor(bdd_op_and(r2,b1),bdd_op_and(b1,r3)));
  print_transform(bdd_op_xor(bdd_op_and(r2,r1),bdd_op_and(r1,b3)));
  print_transform(bdd_op_xor(bdd_op_and(b2,r1),bdd_op_and(r1,b3)));

  print_transform(bdd_op_and(bdd_op_xor(r2,r1),bdd_op_xor(r1,r3)));
  print_transform(bdd_op_and(bdd_op_xor(r2,b1),bdd_op_xor(b1,r3)));
  print_transform(bdd_op_and(bdd_op_xor(r2,r1),bdd_op_xor(r1,b3)));
  print_transform(bdd_op_and(bdd_op_xor(b2,r1),bdd_op_xor(r1,b3)));

  print_transform(bdd_op_xor(
    bdd_op_xor(bdd_op_xor(b0,r0),r1),
    bdd_op_xor(bdd_op_xor(b1,r1),r2)
  ));

  print_transform(bdd_op_xor(
    bdd_op_xor(bdd_op_xor(b0,r0),r1),
    bdd_op_xor(r0,r1)
  ));

}
