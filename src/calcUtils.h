#ifndef _CALC_UTILS_H_
#define _CALC_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "rowTransform.h"
#include "row.h"
#include "coeff.h"



// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
noRnd_col_t calcUtils_intExpandByD(noRnd_col_t val);

coeff_t calcUtils_totProbeMulteplicity(row_t highest_row);

shift_t calcUtils_maxSharesIn(noRnd_col_t value);


#define IPT_ROW      0
#define IPT_SUBROW   1
#define IPT_UNIQUE   2

inline hash_s_t calcUtils_getHashSize(int ipt){
  switch(ipt){
    case IPT_ROW: return rowTransform_row_hash_size();
    case IPT_SUBROW: return rowTransform_subRow_hash_size();
    case IPT_UNIQUE: return rowTransform_unique_hash_size();
    default: FAIL("BUG: calcUtils_getSize\n")
  }
}

inline hash_s_t calcUtils_getHash(int ipt, row_t row){
  switch(ipt){
    case IPT_ROW: return rowTransform_row_hash(row);
    case IPT_SUBROW: return rowTransform_subRow_hash(row);
    case IPT_UNIQUE: return rowTransform_unique_hash(row);
    default: FAIL("BUG: calcUtils_getHash\n")
  }
}

#define ITERATE_PROBE_(Ipt, Var, Next, Code)  { \
    hash_s_t _size = calcUtils_getHashSize(Ipt); \
    uint8_t *_inited = mem_calloc(sizeof(uint8_t), (_size +7)/8, "calcUtils: ITERATE_PROBE_"); \
    row_t Var = row_first(); \
    do{ \
      hash_s_t _hash = calcUtils_getHash(Ipt, Var); \
      hash_s_t _byte = _hash >> 3; \
      hash_s_t _bit = _hash & 7; \
      if( ((_inited[_byte] >> _bit) & 1) == 0 ){ \
        _inited[_byte] |= 1 << _bit; \
        Code  \
      }  \
    }while(Next(& Var)); \
    mem_free(_inited); \
  }

// additional var 'probeAndOut'
#define ITERATE_PROBE_AND_OUT(ipt, code)  ITERATE_PROBE_(ipt, probeAndOut, row_tryNextProbeAndOut, code)
// additional var 'probe'
#define ITERATE_PROBE(ipt, code) ITERATE_PROBE_(ipt, probe, row_tryNextProbe, code)

// additional var 'ii', 'ii_index'
#define ITERATE_II(code)  { \
    int ii_index = 0;  \
    for(noRnd_col_t ii = 0; ii < NUM_NORND_COLS; ii++){\
      if(calcUtils_maxSharesIn(ii) <= T){ \
        code  \
        ii_index++; \
      } \
    } \
  }

// additional var 'x'
#define ITERATE_X_UND(code)  { \
    for(noRnd_col_t x = 0; x < (1ll << NUM_INS); x++){ \
      code  \
    } \
  }

// additional var 'x'
#define ITERATE_X_ACT(code)  { \
    for(noRnd_col_t x = 0; x < (1ll << D * NUM_INS); x++){ \
      code  \
    } \
  }




#endif // _CALC_UTILS_H_
