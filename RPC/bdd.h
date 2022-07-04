#ifndef _BDD_H_
#define _BDD_H_


#include <stdint.h>
#include "types.h"



typedef uint64_t bdd_t;



bdd_t bdd_val_const(bool val);
bdd_t bdd_val_single(shift_t inputBit);

bdd_t bdd_op_not(bdd_t val);
bdd_t bdd_op_and(bdd_t val0, bdd_t val1);
bdd_t bdd_op_xor(bdd_t val0, bdd_t val1);

void bdd_get_transformWithoutRnd(bdd_t val, fixed_cell_t ret[NUM_NORND_COLS]);

double bdd_dbg_storageFill(void);
double bdd_dbg_hashConflictRate(void);

double bdd_dbg_cacheTurnover(void);
double bdd_dbg_cacheMovementRate(void);


#endif // _BDD_H_
