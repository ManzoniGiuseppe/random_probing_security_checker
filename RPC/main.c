#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"
#include "bdd.h"
#include "gadget.h"
#include "correlationTable.h"
#include "probeComb.h"



#if MAX_COEFF > TOT_MUL_PROBES
  #undef MAX_COEFF
  #define MAX_COEFF TOT_MUL_PROBES
#endif

// ensure evering fits in the chosen types
#if NUM_COLS > INT32_MAX
#error "too many columns"
#endif

#if D*NUM_IN > 64
#error "too many shared input bits"
#endif



#define MAX_FIXED_SUM  (1ll << (NUM_TOT_INS+1))
#define FIXED_SUM_NORMALIZE(x)  ((x) > MAX_FIXED_SUM ? MAX_FIXED_SUM : (x))

// useful def
#define LEAD_1(x) (63 - __builtin_clzll((x)))
#define TAIL_1(x) LEAD_1((x)&-(x))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#define MIN(x,y)  ((x) < (y) ? (x) : (y))



static void calculateAll(){
  bdd_t x[NUM_TOT_INS];
  for(shift_t i = 0; i < NUM_TOT_INS; i++){
    x[i] = bdd_val_single(i);
  }
  bdd_t core[NUM_TOT_OUTS];
  gadget_fn(x, core);

  row_t row = row_first();
  do{
    bdd_t row_fn = bdd_val_const(0);

    for(int i = 0; i < ROW_VALUES_SIZE; i++){
      row_value_t xorWith = row.values[i];

      while(xorWith != 0){
        shift_t tail = TAIL_1(xorWith);
        row_fn = bdd_op_xor(row_fn, core[tail]);
        xorWith ^= 1ll << tail;
      }
    }

    fixed_cell_t transform[NUM_NORND_COLS];
    bdd_get_transformWithoutRnd(row_fn, transform);
    correlationTable_row_insertTransform(row, transform);
  }while(row_tryNextProbeAndOut(& row));
}





// calculate with the 'is' formula
static fixed_sum_t rps_is(probeComb_t probes){ // returns with fixed point notation.
  row_t row = probeComb_getHighestRow_noOut(probes);
//printf("rps_is: row %lx\n", row.values[0]);
  return correlationTable_probe_getRPS(row) != 0 ? MAX_FIXED_SUM : 0;
}

// calculate with the 'sum' formula
static fixed_sum_t rps_sum(probeComb_t probes){ // returns with fixed point notation.
  shift_t shift_by = probeComb_getRowMulteplicity(probes);
  fixed_sum_t ret = correlationTable_probe_getRPS(probeComb_getHighestRow_noOut(probes));

  if(ret == 0) return 0; // to avoid bug in the next line if  MAX_FIXED_SUM >> shift_by == 0
  if(ret >= (MAX_FIXED_SUM >> shift_by) ) return MAX_FIXED_SUM; // nearly the same as asking if  (ret << shift_by) >= MAX_FIXED_SUM

  return ret << shift_by;
}

// calculate the coefficient of the f(p) using evalCombination to get the individual coefficient of each combination of probes.
static void min_f_rps(double *retCoeffs, fixed_sum_t (*evalCombination)(probeComb_t)){
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    retCoeffs[i] = 0.0;
  }

  ITERATE_OVER_PROBES(probes, coeffNum, {
    double mul = probeComb_getProbesMulteplicity(probes);
    fixed_sum_t r = evalCombination(probes);
    retCoeffs[coeffNum] += r / ((double) (1ll << (NUM_TOT_INS+1))) * mul;
  })
}



// calculate with the 'is' formula
static fixed_sum_t rpc_is(probeComb_t probes, row_t output){ // returns with fixed point notation.
  row_t highest_row = probeComb_getHighestRow(probes, output);
  return correlationTable_probe_getRPC(highest_row) != 0 ? MAX_FIXED_SUM : 0;
}

// calculate with the 'sum' formula
static fixed_sum_t rpc_sum(probeComb_t probes, row_t output){ // returns with fixed point notation.
  shift_t shift_by = probeComb_getRowMulteplicity(probes);
  row_t highest_row = probeComb_getHighestRow(probes, output);
  fixed_sum_t ret = correlationTable_probe_getRPC(highest_row);

//if(highest_row.values[0] == 0x60){
//printf("rpc: out=%x, probes={", (int) output.values[0]);
//for(int i = 0; i < NUM_PROBES; i++)
//  printf("%d, ", probes[i]);
//printf("} ret=%ld\n", (long) ret);
//}

  if(ret == 0) return 0; // to avoid bug in the next line if  MAX_FIXED_SUM >> shift_by == 0
  if(ret >= (MAX_FIXED_SUM >> shift_by) ) return MAX_FIXED_SUM; // nearly the same as asking if  (ret << shift_by) >= MAX_FIXED_SUM

  return ret << shift_by;
}

static double calculateF(double *coeffs, double p){
  double ret = 0.0;
  double pi;
  shift_t i;
  for(pi = 1.0, i = 0; i <= MAX_COEFF; i++, pi *= p){
    ret += coeffs[i] * pi;
  }
  return MIN(1, ret);
}

// calculate the coefficient of the f(p) using evalCombination to get the individual coefficient of each combination of probes.
static void min_f_rpc(double *retCoeffs, fixed_sum_t (*evalCombination)(probeComb_t, row_t), bool *ret_wasApproximated){
  *ret_wasApproximated = 0;

  for(shift_t i = 0; i <= MAX_COEFF; i++)
    retCoeffs[i] = 0.0;

  row_t output = row_first();
  do{
    if(row_maxShares(output) < T) continue; // the max will be in the ones with most outputs
    double coeffs[MAX_COEFF+1] = {0.0};

    ITERATE_OVER_PROBES(probes, coeffNum, {
//printf("rpc: out=%x, probes={", (int) output.values[0]);
//for(int i = 0; i < NUM_PROBES; i++)
//  printf("%d, ", probes[i]);
//printf("} row=%lx, sum=%d", (long) probeComb_getHighestRow_noOut(probes).values[0], coeffNum);

      fixed_sum_t r = evalCombination(probes, output);
      if(r != 0){
        double mul = probeComb_getProbesMulteplicity(probes);
//printf(", mul=%f", mul);
//printf(", r=%lld\n", (long long) r);
        coeffs[coeffNum] += r / ((double) (1ll << (NUM_TOT_INS+1))) * mul;
      }
    })

    bool gt = 0;
    bool lt = 0;
    for(shift_t i = 0; i <= MAX_COEFF; i++){
      if(coeffs[i] > retCoeffs[i]) gt = 1;
      if(coeffs[i] < retCoeffs[i]) lt = 1;
    }

    if(!gt) continue;
    if(gt && !lt){
      for(shift_t i = 0; i <= MAX_COEFF; i++)
        retCoeffs[i] = coeffs[i];
      *ret_wasApproximated = 0;
      continue;
    }
    // gt && lt

    // try again more accurately
    gt = lt = 0;
    for(double p = 0.0; p <= 1.0; p+=FN_CMP_STEP){
      double rcf = calculateF(retCoeffs, p);
      double cf = calculateF(coeffs, p);
      if(cf > rcf) gt = 1;
      if(cf < rcf) lt = 1;
    }

    if(!gt) continue;
    if(gt && !lt){
      for(shift_t i = 0; i <= MAX_COEFF; i++)
        retCoeffs[i] = coeffs[i];
      *ret_wasApproximated = 0;
      continue;
    }
    // gt && lt
    for(shift_t i = 0; i <= MAX_COEFF; i++)
      retCoeffs[i] = MAX(retCoeffs[i], coeffs[i]); // approximate maximum f by using the maximum coefficients.

    *ret_wasApproximated = 1;
  }while(row_tryNextOut(& output));
}




// output the info.
int main(){
  printf("program started\n");

  printf("calculating matrix info...\n");
  calculateAll();
  printf("info calculated\n");


  double retCoeff[MAX_COEFF+1];

  min_f_rps(retCoeff, rps_is);
  printf("RPS: coeffs of M0:   ");
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  min_f_rps(retCoeff, rps_sum);
  printf("RPS: coeffs of Mgm:  ");
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  bool ret_wasApproximated;

  min_f_rpc(retCoeff, rpc_is, & ret_wasApproximated);
  if(ret_wasApproximated)
    printf("RPC: coeffs of ~M0:  ");
  else
    printf("RPC: coeffs of M0:  ");
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  min_f_rpc(retCoeff, rpc_sum, & ret_wasApproximated);
  if(ret_wasApproximated)
    printf("RPC: coeffs of ~Mgm:  ");
  else
    printf("RPC: coeffs of Mgm:  ");
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  printf("BDD, used=%f%%\n", bdd_dbg_storageFill() * 100);
  printf("BDD, hashConflict=%f%%\n",  bdd_dbg_hashConflictRate() * 100);
  printf("CorrelationTable, storage: used=%f%%\n",  correlationTable_dbg_storageFill() * 100);
  printf("CorrelationTable, storage: hashConflict=%f%%\n",  correlationTable_dbg_storageHashConflictRate() * 100);
  printf("CorrelationTable, assoc: used=%f%%\n",  correlationTable_dbg_assocFill() * 100);
  printf("CorrelationTable, assoc: hashConflict=%f%%\n",  correlationTable_dbg_assocHashConflictRate() * 100);


  return 0;
}


// TODO: check limits due to types.
// TODO: improve bdd complexities with caches.

// TODO: fix multiple outputs, bugs everywhere
// TODO: speed up
