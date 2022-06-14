#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>


// the real ones must be prepended
//#define NUM_INS 2
//#define NUM_OUTS 1
//#define D 3
//#define NUM_RANDOMS 5
//#define NUM_PROBES 5
//#define MAX_COEFF 3


#define NUM_TOT_INS (NUM_INS * D + NUM_RANDOMS)
#define NUM_TOT_OUTS (NUM_OUTS* D + NUM_PROBES)

#define NUM_COLS (1ll << NUM_TOT_INS)
#define NUM_ROWS (1ll << NUM_TOT_OUTS)


// ensure the program works
#if NUM_COLS > INT32_MAX
#error "too many columns"
#endif

#if NUM_TOT_INS + NUM_TOT_OUTS > 63
#error "too big for the fixed point notation"
#endif

#if MAX_COEFF > NUM_PROBES
#error MAX_COEFF "can go up to" NUM_PROBES
#endif

// the stdbool.h definition is just a waste of space
typedef uint8_t bool;

// a plain row of the correlation matrix
typedef struct matrix_row{
  bool wasInited;
  bool plain[NUM_COLS]; // the xored outputs of the function
  int32_t transformed[NUM_COLS];  // the actual value is >> NUM_TOT_INS   // the transformed outputs: the actual row
} matrix_row_t;

#define LEAD_1(x) (63 - __builtin_clzll((x)))
#define TAIL_1(x) LEAD_1((x)&-(x))

// root of the tree containing the matrix
void * matrix[64];

// traverse the tree to get the row (internal)
void * getPos_i(void** from, uint64_t pos){
  if(pos == 0) return from;

  int32_t tail = TAIL_1(pos); // pos=0 handled
  if(pos >> tail == 1){ // leaf
    if(!from[tail]){
      from[tail] = calloc(1, sizeof(matrix_row_t));
    }
    return from[tail];
  }else{ // node
    if(!from[tail]){
      from[tail] = calloc(64, sizeof(void*));
    }
    return getPos_i((void**) from[tail], pos >> (tail+1));
  }
}

// get the row of the matrix
matrix_row_t* getPos(uint64_t pos){
  pos |= 1ll << 63;
  return getPos_i(matrix, pos);
}


// definition of the gadget, to be appended afterward.
void gadget(bool x[NUM_TOT_INS], bool ret[NUM_TOT_OUTS]);

// Fast Walsh-Hadamard transform
void inPlace_transform(int32_t *tr, int32_t size){ // tr must have 1 for false, -1 for true
  if (size == 1) return;
  int32_t h = size/2;
  for (int32_t i = 0; i < h; i++){
    int32_t a = tr[i];
    int32_t b = tr[i+h];
    tr[i] = a + b;
    tr[i+h] = a - b;
  }
  inPlace_transform(tr, h);
  inPlace_transform(tr + h, h);
}

// after setting plain to its value, call this to set the rest
void finalizze_row(matrix_row_t* row){
  int32_t val[2] = {1, -1};
  for(int32_t i =0; i < NUM_COLS; i++){
    row->transformed[i] = val[row->plain[i]];
  }
  inPlace_transform(row->transformed, NUM_COLS);
  row->wasInited = 1;
}

// calculate the basic rows needed to set up the rest of the matrix
void calculateCore(){
  matrix_row_t* r = getPos(0);
  for(int32_t i = 0; i < NUM_COLS; i++){
    r->plain[i] = 0;
  }
  finalizze_row(r);

  bool x[NUM_TOT_INS];
  bool ret[NUM_TOT_OUTS];
  for(int32_t i = 0; i < NUM_COLS; i++){
    for(int32_t j = 0; j < NUM_TOT_INS; j++){
      x[j] = (i >> j) & 1;
    }
    gadget(x, ret);
    for(int32_t j = 0; j < NUM_TOT_OUTS; j++){
      getPos(1ll << j)->plain[i] = ret[j];
    }
  }
  for(int32_t j = 0; j < NUM_TOT_OUTS; j++){
    finalizze_row(getPos(1ll << j));
  }
}


// get (and lazily initialize) the actual row of the matrix (internal)
matrix_row_t *getRow_i(int64_t row){
  matrix_row_t* r = getPos(row);
  if(!r->wasInited){
    int64_t topmost = 1ll << LEAD_1(row); // row=0 has been calculated
    bool *first = getPos(topmost)->plain;
    bool *second = getRow_i(row ^ topmost)->plain;
    for(int32_t j = 0; j < NUM_COLS; j++){
      r->plain[j] = first[j] ^ second[j];
    }
    finalizze_row(r);
  }
  return r;
}

// get (and lazily initialize) the actual row of the matrix
#define getRow(row) (getRow_i((row))->transformed)

// used as an iterator to get the next sub probe.
int64_t getNextSubProbe(int64_t probes, int64_t sub){ // first is 0, return 0 for end
  if(sub == probes) return 0;
  int64_t lowest = 1ll << TAIL_1(sub ^ probes);  // sub ^ probes == 0 <-> sub == probes, which has been handled.
  return (sub & ~(lowest-1)) | lowest;
}

// with d=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
int64_t int_expand_by(int64_t val, int64_t d){
  if(val == 0) return 0;
  int64_t bit = (1ll<<d)-1;
  int64_t ret = 0;
  for(int64_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << i*d;
  }
  return ret;
}

// calculate with the 'is' formula
int64_t rps_is(int64_t probes){ // returns with fixed point notation.
  int64_t w = 0;
  do{
    for(int64_t i = 1; i < (1ll<<NUM_INS); i++){
      if(getRow(w<<(D*NUM_OUTS))[int_expand_by(i,D)] != 0){
        return 1ll << NUM_TOT_INS;
      }
    }
  }while((w = getNextSubProbe(probes, w)));
  return 0;
}

// calculate with the 'sum' formula
int64_t rps_sum(int64_t probes){ // returns with fixed point notation.
  int64_t ret = 0;
  int64_t w = 0;
  do{
    for(int64_t i = 1; i < (1ll<<NUM_INS); i++){
      ret += llabs(getRow(w<<(D*NUM_OUTS))[int_expand_by(i,D)]);
    }
  }while((w = getNextSubProbe(probes, w)));
  ret/=2;
  if(ret > (1ll<<NUM_TOT_INS)) ret = 1ll << NUM_TOT_INS;
  return ret;
}

// iterator to get all the rows with less than MAX_COEFF probes.
int64_t getNextProbe(int64_t probe){ /// 0 is the first probe
  if(__builtin_popcountll(probe+1) <= MAX_COEFF) return probe+1;
  if(probe == 0) return MAX_COEFF != 0 ? 1 : 0;

  int64_t lowest = 1ll << TAIL_1(probe);  // probes == 0 has been handled.
  probe += lowest;
  probe &= ~(1ll << NUM_PROBES); // if overflows return 0 i.e. loop back.
  return probe;
}

// calculate the coefficient of the f(p) using evalCombination to get the individual coefficient of each combination of probes.
void min_failure_probability(float *retCoeff, int64_t (*evalCombination)(int64_t)){
  for(int64_t i = 0; i <= MAX_COEFF; i++){
    retCoeff[i] = 0.0;
  }

  int64_t probes = 0;
  do{
    int coeffNum = __builtin_popcountll(probes);
    retCoeff[coeffNum] += evalCombination(probes) / ((double) (1ll << NUM_TOT_INS));
  }while((probes = getNextProbe(probes))); // the first of the loop back is 0
}


// output the info.
int main(){
  printf("program started\n");

  printf("calculating core...\n");
  calculateCore();
  printf("core calculated\n");

  float retCoeff[MAX_COEFF+1] = {0};
  min_failure_probability(retCoeff, rps_is);
  printf("coeffs of is: ");
  for(int64_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  min_failure_probability(retCoeff, rps_sum);
  printf("coeffs of sum: ");
  for(int64_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  return 0;
}
