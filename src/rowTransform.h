#ifndef _ROWTRANSFORM_H_
#define _ROWTRANSFORM_H_


#include "types.h"
#include "row.h"


void rowTransform_insert(row_t row, fixed_cell_t transform[NUM_NORND_COLS]);

void rowTransform_finalizze(void);

void rowTransform_get(row_t row, fixed_cell_t ret_transform[NUM_NORND_COLS]);
hash_s_t rowTransform_transform_hash_size(void);
hash_s_t rowTransform_transform_hash(row_t row);
row_t rowTransform_transform_anyRow(hash_s_t hash);
hash_s_t rowTransform_row_hash_size(void);
hash_s_t rowTransform_row_hash(row_t row);
row_t rowTransform_row_anyRow(hash_s_t hash);


double rowTransform_transform_dbg_fill(void);
double rowTransform_transform_dbg_hashConflictRate(void);
double rowTransform_row_dbg_allocRate(void);
double rowTransform_assoc_dbg_fill(void);
double rowTransform_assoc_dbg_hashConflictRate(void);


#endif // _ROWTRANSFORM_H_
