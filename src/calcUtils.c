#include "calcUtils.h"
#include "probeComb.h"
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


#define ITERATE_PROBE_AND_OUT(code)  { \
    /* TODO hash_s_t max_h = rowElseProbe ? rowTransform_transform_hash_size() : rowTransform_row_hash_size(); \
    for(hash_s_t h = 0; h < max_h; h++){ \
      row_t probeAndOutput = rowElseProbe ? rowTransform_transform_anyRow(h) : rowTransform_row_anyRow(h); \
      code \
    } */ \
    row_t probeAndOutput = row_first(); \
    do{ \
      code  \
    }while(row_tryNextProbeAndOut(& probeAndOutput)); \
  }

#define ITERATE_PROBE(code)  { \
    row_t probe = row_first(); \
    do{ \
      code  \
    }while(row_tryNextProbe(& probe)); \
  }

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



void calcUtils_init_rowIi(bool rowElseProbe, void (*init)(row_t,col_t,int)){
  ITERATE_PROBE_AND_OUT({
    ITERATE_II({
        init(probeAndOutput, ii, ii_index);
    })
  })
}

void calcUtils_init_rowIiX(bool rowElseProbe, void (*init)(row_t,col_t,int,col_t)){
  ITERATE_PROBE_AND_OUT({
    ITERATE_II({
      ITERATE_X({
        init(probeAndOutput, ii, ii_index, x);
      })
    })
  })
}

void calcUtils_init_rowOIiX(bool rowElseProbe, void (*init)(row_t,row_t,int,col_t,int,col_t)){
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
