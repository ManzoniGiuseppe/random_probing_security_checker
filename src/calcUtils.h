#ifndef _CALC_UTILS_H_
#define _CALC_UTILS_H_

#include "types.h"
#include "row.h"
#include "coeff.h"



// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
col_t calcUtils_intExpandByD(col_t val);

coeff_t calcUtils_totProbeMulteplicity(row_t highest_row);

shift_t calcUtils_maxSharesIn(col_t value);



#define ITERATE_PROBE_(RowElseProbe, Var, Next, Code)  { \
    hash_s_t _size = RowElseProbe ? rowTransform_transform_hash_size() : rowTransform_row_hash_size(); \
    uint8_t *_inited = mem_calloc(sizeof(uint8_t), (_size +7)/8, "calcUtils: ITERATE_PROBE_AND_OUT"); \
    row_t Var = row_first(); \
    do{ \
      hash_s_t _h = RowElseProbe ? rowTransform_transform_hash(Var) : rowTransform_row_hash(Var); \
      hash_s_t _byte = _h >> 3; \
      hash_s_t _bit = _h & 7; \
      if( ((_inited[_byte] >> _bit) & 1) == 0 ){ \
        _inited[_byte] |= 1 << _bit; \
        Code  \
      } \
    }while(Next(& Var)); \
    mem_free(_inited); \
  }

#define ITERATE_PROBE_AND_OUT(RowElseProbe, code)  ITERATE_PROBE_(RowElseProbe, probeAndOutput, row_tryNextProbeAndOut, code)
#define ITERATE_PROBE(RowElseProbe, code) ITERATE_PROBE_(RowElseProbe, probe, row_tryNextProbe, code)

#define ITERATE_II(code)  { \
    int ii_index = 0;  \
    for(col_t ii = 0; ii < NUM_NORND_COLS; ii++){\
      if(calcUtils_maxSharesIn(ii) <= T){ \
        code  \
        ii_index++; \
      } \
    } \
  }

#define ITERATE_X(code)  { \
    for(col_t x = 0; x < (1ll << NUM_INS); x++){ \
      code  \
    } \
  }




#endif // _CALC_UTILS_H_
