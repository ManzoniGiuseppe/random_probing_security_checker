#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"
#include "bdd.h"
#include "calc.h"
#include "coeff.h"
#include "rowTransform.h"
#include "gadget.h"
#include "probeComb.h"



// TODO: move this away from here
#if MAX_COEFF > TOT_MUL_PROBES
  #undef MAX_COEFF
  #define MAX_COEFF TOT_MUL_PROBES
#endif

#if D*NUM_IN > 64
#error "too many shared input bits"
#endif




static void calculateAllRowTransform(){
  printf("calculating row transforms\n");

  void* bdd = bdd_storage_alloc();

  bdd_t x[NUM_TOT_INS];
  for(shift_t i = 0; i < NUM_TOT_INS; i++){
    x[i] = bdd_val_single(bdd, i);
  }
  bdd_t core[NUM_TOT_OUTS];
  gadget_fn(bdd, x, core);

  row_t row = row_first();
  do{
    bdd_t row_fn = bdd_val_const(bdd, 0);

    for(int i = 0; i < ROW_VALUES_SIZE; i++){
      row_value_t xorWith = row.values[i];

      while(xorWith != 0){
        shift_t tail = TAIL_1(xorWith);
        row_fn = bdd_op_xor(bdd, row_fn, core[tail + i * 64]);
        xorWith ^= 1ll << tail;
      }
    }

    fixed_cell_t transform[NUM_NORND_COLS];
    bdd_get_transformWithoutRnd(bdd, row_fn, transform);
    rowTransform_insert(row, transform);
  }while(row_tryNextProbeAndOut(& row));

  printf("BDD, storage: used=%f%%\n", bdd_dbg_storageFill(bdd) * 100);
  printf("BDD, storage: hashConflict=%f%%\n",  bdd_dbg_hashConflictRate(bdd) * 100);
  printf("BDD, cache: turnover=%f%%\n", bdd_dbg_cacheTurnover(bdd) * 100);
  printf("BDD, cache: movement=%f%%\n",  bdd_dbg_cacheMovementRate(bdd) * 100);

  printf("rowTransform, transform: used=%f%%\n",  rowTransform_transform_dbg_fill() * 100);
  printf("rowTransform, transform: hashConflict=%f%%\n",  rowTransform_transform_dbg_hashConflictRate() * 100);
  printf("rowTransform, row: used=%f%%\n",  rowTransform_row_dbg_fill() * 100);
  printf("rowTransform, row: hashConflict=%f%%\n",  rowTransform_row_dbg_hashConflictRate() * 100);
  printf("rowTransform, assoc: used=%f%%\n",  rowTransform_assoc_dbg_fill() * 100);
  printf("rowTransform, assoc: hashConflict=%f%%\n",  rowTransform_assoc_dbg_hashConflictRate() * 100);

  bdd_storage_free(bdd);
}


void print_coeff(coeff_t c, char *type, char *model, char *padding){
  printf("%s: coeffs of %s%s:%s", type, c.approximated ? "~" : "", model, padding);
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", c.values[i]);
  }
  printf("\n");
}

// output the info.
int main(){
  printf("program started\n");

  calculateAllRowTransform();

  print_coeff(calc_rpsIs(),  "RPS", "M0", "   ");
  print_coeff(calc_rpsSum(), "RPS", "Mgm", "  ");
  print_coeff(calc_rpcIs(),  "RPC", "M0", "   ");
  print_coeff(calc_rpcSum(), "RPC", "Mgm", "  ");
  print_coeff(calc_rpcTeo(), "RPC", "Mteo", " ");

  printf("rowTransform, transform: used=%f%%\n",  rowTransform_transform_dbg_fill() * 100);
  printf("rowTransform, transform: hashConflict=%f%%\n",  rowTransform_transform_dbg_hashConflictRate() * 100);
  printf("rowTransform, row: used=%f%%\n",  rowTransform_row_dbg_fill() * 100);
  printf("rowTransform, row: hashConflict=%f%%\n",  rowTransform_row_dbg_hashConflictRate() * 100);
  printf("rowTransform, assoc: used=%f%%\n",  rowTransform_assoc_dbg_fill() * 100);
  printf("rowTransform, assoc: hashConflict=%f%%\n",  rowTransform_assoc_dbg_hashConflictRate() * 100);


  return 0;
}


// TODO: check limits due to types.
