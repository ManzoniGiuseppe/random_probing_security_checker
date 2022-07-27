#ifndef _CALC_UTILS_H_
#define _CALC_UTILS_H_

#include "types.h"
#include "row.h"
#include "coeff.h"



// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
col_t calcUtils_intExpandByD(col_t val);

coeff_t calcUtils_totProbeMulteplicity(row_t highest_row);

shift_t calcUtils_maxSharesIn(col_t value);

#endif // _CALC_UTILS_H_
