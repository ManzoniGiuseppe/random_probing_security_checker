#ifndef _ROWTRANSFORM_H_
#define _ROWTRANSFORM_H_


#include "types.h"
#include "row.h"


void rowTransform_insert(row_t row, fixed_cell_t transform[NUM_NORND_COLS]);

void rowTransform_finalizze(void);

void rowTransform_get(row_t row, fixed_cell_t ret_transform[NUM_NORND_COLS]);

hash_s_t rowTransform_row_hash_size(void);
hash_s_t rowTransform_row_hash(row_t row);

hash_s_t rowTransform_subRow_hash_size(void);
hash_s_t rowTransform_subRow_hash(row_t row);

hash_s_t rowTransform_unique_hash_size(void);
hash_s_t rowTransform_unique_hash(row_t row);


double rowTransform_row_dbg_fill(void);
double rowTransform_row_dbg_fillOfTot(void);
double rowTransform_row_dbg_hashConflictRate(void);
double rowTransform_subRow_dbg_allocRate(void);
double rowTransform_unique_dbg_fill(void);
double rowTransform_unique_dbg_hashConflictRate(void);


#endif // _ROWTRANSFORM_H_
