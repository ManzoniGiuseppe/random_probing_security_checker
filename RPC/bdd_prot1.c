#include <stdio.h>
#include <stdlib.h>

#include "bdd.h"
#include "types.h"

// INFO: only for small gadgets, inefficient plain storage


#define STORAGE_MAX ((1<<15)-1)
#define FAIL(...)  { fprintf(stderr, __VA_ARGS__); exit(1); }

static int storage_size;
static bool storage[STORAGE_MAX + 1][1ll << NUM_TOT_INS];

static void storage_init(void){
  static bool inited = 0;
  if(inited) return;

  for(int i = 0; i < (1ll<<NUM_TOT_INS); i++){
    storage[0][i] = 0; // false
    storage[1][i] = 1; // true

    for(int j = 0; j < NUM_TOT_INS; j++){
      storage[2 + j][i] = (i & (1ll << j)) != 0; // the save value as the selected input bit
    }
  }
  storage_size = 2 + NUM_TOT_INS;
  inited = 1;
}

double bdd_dbg_storageFill(void){
  return ((double) storage_size) / (STORAGE_MAX + 1);
}
double bdd_dbg_hashConflictRate(void){
  return 0.0;
}



bdd_t bdd_val_const(bool val){
  return val ? 1 : 0;
}

bdd_t bdd_val_single(shift_t inputBit){
  return 2 + inputBit;
}

bdd_t bdd_op_not(bdd_t val){
  storage_init();
  for(int i = 0; i < (1ll<<NUM_TOT_INS); i++){
    storage[storage_size][i] = ~storage[val][i];
  }
  if(storage_size == STORAGE_MAX) FAIL("bdd_op_not: no space left.")
  return storage_size++;
}

bdd_t bdd_op_and(bdd_t val0, bdd_t val1){
  storage_init();
  for(int i = 0; i < (1ll<<NUM_TOT_INS); i++){
    storage[storage_size][i] = storage[val0][i] & storage[val1][i];
  }
  if(storage_size == STORAGE_MAX) FAIL("bdd_op_and: no space left.")
  return storage_size++;
}
bdd_t bdd_op_xor(bdd_t val0, bdd_t val1){
  storage_init();
  for(int i = 0; i < (1ll<<NUM_TOT_INS); i++){
    storage[storage_size][i] = storage[val0][i] ^ storage[val1][i];
  }
  if(storage_size == STORAGE_MAX) FAIL("bdd_op_xor: no space left.")
  return storage_size++;
}

// Fast Walsh-Hadamard transform
static void inPlaceTransform(fixed_cell_t *tr, col_t size){ // tr must have 1 for false, -1 for true, can accept partially transformed inputs too
  if (size == 1) return;
  col_t h = size/2;
  for (col_t i = 0; i < h; i++){
    fixed_cell_t a = tr[i];
    fixed_cell_t b = tr[i+h];
    tr[i] = a + b;
    tr[i+h] = a - b;
  }
  inPlaceTransform(tr, h);
  inPlaceTransform(tr + h, h);
}


void bdd_get_transformWithoutRnd(bdd_t val, fixed_cell_t ret[NUM_NORND_COLS]){
  storage_init();
  fixed_cell_t t[1ll<<NUM_TOT_INS];
  for(int i = 0; i < (1ll<<NUM_TOT_INS); i++){
    t[i] = storage[val][i] ? -1 : 1;
  }

  inPlaceTransform(t, 1ll << NUM_TOT_INS);

  for(int i = 0; i < NUM_NORND_COLS; i++){
    ret[i] = t[i];
  }
}
