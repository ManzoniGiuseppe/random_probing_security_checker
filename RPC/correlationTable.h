#ifndef _CORRELATIONTABLE_H_
#define _CORRELATIONTABLE_H_


#include "types.h"
#include "row.h"


void correlationTable_row_insertTransform(row_t row, fixed_cell_t transform[NUM_NORND_COLS]);

// first insert all rows, in any order, then get the probe info.

fixed_sum_t correlationTable_probe_getRPS(row_t row);
fixed_sum_t correlationTable_probe_getRPC(row_t row);

double correlationTable_dbg_storageFill(void);
double correlationTable_dbg_storageHashConflictRate(void);
double correlationTable_dbg_assocFill(void);
double correlationTable_dbg_assocHashConflictRate(void);




#endif // _CORRELATIONTABLE_H_

