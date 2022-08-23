#include "calcUtils.h"
#include "probeComb.h"
#include "mem.h"
#include "row.h"
#include "rowTransform.h"


// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
col_t calcUtils_intExpandByD(col_t val){
  if(val == 0) return 0;
  col_t bit = (1ll<<D)-1;
  col_t ret = 0;
  for(shift_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << (i*D);
  }
  return ret;
}

coeff_t calcUtils_totProbeMulteplicity(row_t row){
  coeff_t ret = coeff_zero();
  ITERATE_OVER_PROBES_WHOSE_IMAGE_IS(row, probeComb, coeffNum, {
    double mul = probeComb_getProbesMulteplicity(probeComb);
    ret.values[coeffNum] += mul;
  })
  return ret;
}

shift_t calcUtils_maxSharesIn(col_t value){
  shift_t ret = 0;
  col_t mask = (1ll << D)-1;
  for(int i = 0; i < NUM_INS; i++){
    int v = __builtin_popcountll(value & (mask << (i*D)));
    ret = v > ret ? v : ret;
  }
  return ret;
}


#define ITERATE_PROBE_(Var, Next, Code)  { \
    hash_s_t _size = rowElseProbe ? rowTransform_transform_hash_size() : rowTransform_row_hash_size(); \
    uint8_t *_inited = mem_calloc(sizeof(uint8_t), (_size +7)/8, "calcUtils: ITERATE_PROBE_AND_OUT"); \
    row_t Var = row_first(); \
    do{ \
      hash_s_t _h = rowElseProbe ? rowTransform_transform_hash(Var) : rowTransform_row_hash(Var); \
      hash_s_t _byte = _h >> 3; \
      hash_s_t _bit = _h & 7; \
      if( ((_inited[_byte] >> _bit) & 1) == 0 ){ \
        _inited[_byte] |= 1 << _bit; \
        Code  \
      } \
    }while(Next(& Var)); \
    mem_free(_inited); \
  }

#define ITERATE_PROBE_AND_OUT(code)  ITERATE_PROBE_(probeAndOutput, row_tryNextProbeAndOut, code)
#define ITERATE_PROBE(code) ITERATE_PROBE_(probe, row_tryNextProbe, code)

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

#define ITERATE_O(row, code)  { \
    row_t o = row_first(); \
    int o_index = 0; \
    do{ \
      code \
      o_index++; \
    }while(row_tryGetNext(row, & o)); \
  }



void calcUtils_init_outIi(bool rowElseProbe, void (*init)(row_t,col_t,int)){
  ITERATE_PROBE_AND_OUT({
    ITERATE_II({
        init(probeAndOutput, ii, ii_index);
    })
  })
}

void calcUtils_init_outIiX(bool rowElseProbe, void (*init)(row_t,col_t,int,col_t)){
  ITERATE_PROBE_AND_OUT({
    ITERATE_II({
      ITERATE_X({
        init(probeAndOutput, ii, ii_index, x);
      })
    })
  })
}

void calcUtils_init_outOIiX(bool rowElseProbe, void (*init)(row_t,row_t,int,col_t,int,col_t)){
  ITERATE_PROBE_AND_OUT({
    ITERATE_O(probeAndOutput, {
      ITERATE_II({
        ITERATE_X({
          init(probeAndOutput, o, o_index, ii, ii_index, x);
        })
      })
    })
  })
}

void calcUtils_init_probe(bool rowElseProbe, void (*init)(row_t)){
  ITERATE_PROBE({
    init(probe);
  })
}

void calcUtils_init_probeX(bool rowElseProbe, void (*init)(row_t,col_t)){
  ITERATE_PROBE({
    ITERATE_X({
      init(probe, x);
    })
  })
}

void calcUtils_init_probeOX(bool rowElseProbe, void (*init)(row_t,row_t,int,col_t)){
  ITERATE_PROBE({
    ITERATE_O(probe, {
      ITERATE_X({
        init(probe, o, o_index, x);
      })
    })
  })
}
