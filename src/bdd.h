#ifndef _BDD_H_
#define _BDD_H_


#include <stdint.h>
#include "types.h"



typedef uint64_t bdd_t;


void* bdd_storage_alloc();
void bdd_storage_free(void*);

bdd_t bdd_val_const(void* storage, bool val);
bdd_t bdd_val_single(void* storage, shift_t inputBit);

bdd_t bdd_op_not(void* storage, bdd_t val);
bdd_t bdd_op_and(void* storage, bdd_t val0, bdd_t val1);
bdd_t bdd_op_xor(void* storage, bdd_t val0, bdd_t val1);

void bdd_get_transformWithoutRnd(void* storage, bdd_t val, fixed_cell_t ret[NUM_NORND_COLS]);

double bdd_dbg_storageFill(void* storage);
double bdd_dbg_hashConflictRate(void* storage);

double bdd_dbg_cacheTurnover(void* storage);
double bdd_dbg_cacheMovementRate(void* storage);


#endif // _BDD_H_
