#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


// the real ones must be prepended:
//
//#define NUM_INS 2
//#define NUM_OUTS 1
//#define D 2
//#define NUM_RANDOMS 1
//#define NUM_PROBES 11
//#define MAX_COEFF 3
//
//#define TOT_MUL_PROBES 21
//
//int multeplicity[NUM_PROBES] = {1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3};


#define NUM_TOT_INS (NUM_INS * D + NUM_RANDOMS)
#define NUM_TOT_OUTS (NUM_OUTS* D + NUM_PROBES)

#define NUM_COLS (1ll << NUM_TOT_INS)
#define NUM_ROWS (1ll << NUM_TOT_OUTS)

#define NUM_NORND_COLS (1ll << (NUM_INS * D))


// ensure evering fits in the chosen types
#if NUM_COLS > INT32_MAX
#error "too many columns"
#endif

#if MAX_COEFF > TOT_MUL_PROBES
#error MAX_COEFF "can go up to" TOT_MUL_PROBES
#endif



typedef uint8_t bool;
typedef uint64_t row_t;
typedef uint32_t col_t;
typedef int32_t shift_t;
typedef uint64_t bitarray_t;
typedef int32_t fixed_cell_t;  // fixed point notation 0.NUM_TOT_INS
typedef uint64_t fixed_sum_t; // fixed point notation 1.(NUM_TOT_INS+1)

// useful def
#define LEAD_1(x) (63 - __builtin_clzll((x)))
#define TAIL_1(x) LEAD_1((x)&-(x))


//-- define a tree to store all the matrix_row_t. Use getRow(row) to get a location as if it was an array instead of a tree


// a row of the correlation matrix
typedef struct matrix_row{
  bool wasInited;
  bitarray_t plain[(NUM_COLS+63) / 64]; // the xored outputs of the function
  fixed_cell_t transformed[NUM_NORND_COLS];  // the transformed outputs: the actual row. it doesn't save the ones with the coordinates of the randoms \neq 0 as they're never used.
} matrix_row_t;


#define GET_PLAIN(arr, pos)  (((arr)[(pos) / 64] >> ((pos) % 64)) & 1)
#define SET_PLAIN(arr, pos, val)  {(arr)[(pos) / 64] ^= (GET_PLAIN(arr,pos) ^ (val))  << ((pos) % 64);}



// root of the tree containing the matrix
void * matrix[64];

// traverse the tree to get the row (internal)
void * getRow_i(void** from, row_t pos, int depth){
  if(pos == 0) return from;

  shift_t tail = TAIL_1(pos); // pos=0 handled
  if(pos >> tail == 1 || depth == MAX_COEFF){ // natural leaf || it just has no leaves
    if(!from[tail]){
      from[tail] = calloc(1, sizeof(matrix_row_t));
    }
    return from[tail];
  }else{ // node
    if(!from[tail]){
      from[tail] = calloc(LEAD_1(pos)+1, sizeof(void*));
    }
    return getRow_i((void**) from[tail], pos >> (tail+1), depth+1);
  }
}

// get the row of the matrix
matrix_row_t* getRow(row_t pos){
  pos |= 1ll << 63;
  return getRow_i(matrix, pos, 1);
}



//-- define the function to get the correlation matrix: getInitedRow(row)[column]



// definition of the gadget, to be appended afterward.
void gadget(bool x[NUM_TOT_INS], bool ret[NUM_TOT_OUTS]);

// Fast Walsh-Hadamard transform
void inPlace_transform(fixed_cell_t *tr, col_t size){ // tr must have 1 for false, -1 for true
  if (size == 1) return;
  col_t h = size/2;
  for (col_t i = 0; i < h; i++){
    fixed_cell_t a = tr[i];
    fixed_cell_t b = tr[i+h];
    tr[i] = a + b;
    tr[i+h] = a - b;
  }
  inPlace_transform(tr, h);
  if(NUM_NORND_COLS > h){
    inPlace_transform(tr + h, h);
  }
}

// after setting plain to its value, call this to set the rest
void finalizze_row(matrix_row_t* row){
  static const fixed_cell_t val[2] = {1, -1};
  fixed_cell_t transformed[NUM_COLS];
  for(col_t i =0; i < NUM_COLS; i++){
    transformed[i] = val[GET_PLAIN(row->plain, i)];
  }
  inPlace_transform(transformed, NUM_COLS);
  for(col_t i =0; i < NUM_NORND_COLS; i++){
    row->transformed[i] = transformed[i];
  }
  row->wasInited = 1;
}

// calculate the basic rows needed to set up the rest of the matrix. to be called before asking for any row
void calculateCore(){
  matrix_row_t* r = getRow(0);
  for(col_t i = 0; i < NUM_COLS; i++){
    SET_PLAIN(r->plain, i, 0)
  }
  finalizze_row(r);

  matrix_row_t* core[NUM_TOT_OUTS];
  for(shift_t j = 0; j < NUM_TOT_OUTS; j++){
    core[j] = getRow(1ll << j);
  }

  bool x[NUM_TOT_INS];
  bool ret[NUM_TOT_OUTS];
  for(col_t i = 0; i < NUM_COLS; i++){
    for(shift_t j = 0; j < NUM_TOT_INS; j++){
      x[j] = (i >> j) & 1;
    }
    gadget(x, ret);
    for(shift_t j = 0; j < NUM_TOT_OUTS; j++){
      SET_PLAIN(core[j]->plain, i, ret[j])
    }
  }
  for(shift_t j = 0; j < NUM_TOT_OUTS; j++){
    finalizze_row(core[j]);
  }
}

// get (and lazily initialize) the actual row of the matrix (internal)
matrix_row_t *getInitedRow_i(row_t row){
  matrix_row_t* r = getRow(row);
  if(!r->wasInited){
    row_t topmost = 1ll << LEAD_1(row); // row=0 has been calculated
    bitarray_t *first = getRow(topmost)->plain;
    bitarray_t *second = getInitedRow_i(row ^ topmost)->plain;
    for(col_t j = 0; j < NUM_COLS; j++){
      SET_PLAIN(r->plain, j, GET_PLAIN(first, j) ^ GET_PLAIN(second, j))
    }
    finalizze_row(r);
  }
  return r;
}

// get (and lazily initialize) the actual row of the matrix (internal)
#define getInitedRow(row) (getInitedRow_i((row))->transformed)



//-- probes and rows


typedef int prob_comb_t[NUM_PROBES];

row_t getHighestRow(prob_comb_t curr_comb){
  row_t ret=0;
  for(shift_t i = 0; i < NUM_PROBES; i++){
    if(curr_comb[i] > 0){
      ret |= 1ll << i;
    }
  }
  return ret;
}

// used as an iterator to get the next sub probe.
row_t getNextRow(row_t highestRow, row_t curr){ // first is 0, return 0 for end
  if(curr == highestRow) return 0;
  row_t lowest = 1ll << TAIL_1(curr ^ highestRow);  // sub ^ probes == 0 <-> sub == probes, which has been handled.
  return (curr & ~(lowest-1)) | lowest;
}

#define ITERATE_OVER_ROWS_OF_PROBE(curr_comb, iterator, code)  { \
    row_t iterator = 0; \
    row_t iterateOverRowsOfProbe_highestRow = getHighestRow((curr_comb)); \
    do{ \
      code \
    }while((iterator = getNextRow(iterateOverRowsOfProbe_highestRow, iterator))); \
  }

// return the logaritm, how much it needs to be shifted
shift_t get_row_multeplicity(prob_comb_t curr_comb, row_t row){
  // check for bugs
  for(shift_t i = 0; i < NUM_PROBES; i++){
    if( ((row >> i)&1) != 0 && curr_comb[i] == 0 ){
      fprintf(stderr, "BUG! Assert failed\n");
      fprintf(stderr, "  curr_comb: ");
      for(shift_t i = 0; i < NUM_PROBES; i++){
         fprintf(stderr, "%d, ", (int) curr_comb[i]);
      }
      fprintf(stderr, "\n");
      fprintf(stderr, "  row: ");
      for(shift_t i = 0; i < NUM_PROBES; i++){
         fprintf(stderr, "%d, ", (int) (row >> i) & 1);
      }
      fprintf(stderr, "\n");
      exit(1);
    }
  }

  // comb: 1,3,4,0,1
  // row:  0,0,0,0,0

  // 0 0 0 0 0 *(4 0)  = 1
  // 0 2 0 0 0 *(3 2)  = 3
  // 0 0 2 0 0 *(4 2)  = 6
  // 0 0 4 0 0 *(4 4)  = 1
  // 0 2 2 0 0 *(3 2)(4 2) = 3*6 = 18
  // 0 2 4 0 0 *(3 2) = 3
  //             tot = 11+18+3 = 32
  // 1*4*8*1*1 = 32


  long ret=0;
  for(shift_t i = 0; i < NUM_PROBES; i++){
    if(curr_comb[i] > 1){
      ret += curr_comb[i]-1;  // sum of even/odd binomial (n k) is 2^(n-1)
    }
  }

  return ret;
}

bool tryIncrementProbeComb(prob_comb_t curr_comb, shift_t* curr_count){ // inited respectively to all 0s and 0.
  shift_t to_inc = 0;
  if(*curr_count < MAX_COEFF) to_inc = 0; // if there is probes left, increment normally
  else { // already hit the maximum.
    for(shift_t i = 0; i < NUM_PROBES; i++){
      if(curr_comb[i] != 0){ // find lowest position != 0
        *curr_count -= curr_comb[i];  // consider is as if it has hit its maximum and increment it
        curr_comb[i] = 0;
        to_inc = i+1; // ask to increment the next, due to overflow
        break;
      }
    }
  }

  bool carry = 1;
  for(shift_t i = to_inc; i < NUM_PROBES && carry; i++){
    if(curr_comb[i] < multeplicity[i]){ // if can still increment without overflowing
      curr_comb[i] += 1;
      *curr_count += 1;
      carry = 0;
    }else{ // reached maximum
      *curr_count -= curr_comb[i];
      curr_comb[i] = 0;
    }
  }

  return !carry;  // if there is still a carry, we looped back. Otherwise we succeded.
}

#define ITERATE_OVER_PROBES(iterator_comb, iterator_count, code)  { \
    prob_comb_t iterator_comb = {0}; \
    shift_t iterator_count = 0; \
    do{ \
      code \
    }while(tryIncrementProbeComb(iterator_comb, &iterator_count)); \
  }

double binomial(int n, int k){
  if(k > n-k) return binomial(n, n-k);

  double ret = 1.0;
  for(int i = n-k+1; i <= n; i++)
    ret *= i;
  for(int i = 2; i <= k; i++)
    ret /= i;

  return ret;
}

double get_probes_multeplicity(prob_comb_t curr_comb){
  double ret = 1.0;
  for(shift_t i = 0; i < NUM_PROBES; i++){
    ret *= binomial(multeplicity[i], curr_comb[i]);
  }
  return ret;
}






// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
col_t int_expand_by_D(col_t val){
  if(val == 0) return 0;
  col_t bit = (1ll<<D)-1;
  col_t ret = 0;
  for(shift_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << i*D;
  }
  return ret;
}

// calculate with the 'is' formula
fixed_sum_t rps_is(prob_comb_t probes){ // returns with fixed point notation.
  ITERATE_OVER_ROWS_OF_PROBE(probes, row, {
    for(col_t i = 1; i < (1ll<<NUM_INS); i++){
      if(getInitedRow(row<<(D*NUM_OUTS))[int_expand_by_D(i)] != 0){
        return 1ll << (NUM_TOT_INS+1);
      }
    }
  })
  return 0;
}

// calculate with the 'sum' formula
fixed_sum_t rps_sum(prob_comb_t probes){ // returns with fixed point notation.
  fixed_sum_t max_ret = 1ll << (NUM_TOT_INS+1);
  fixed_sum_t ret = 0;
  ITERATE_OVER_ROWS_OF_PROBE(probes, row, {
    for(col_t i = 1; i < (1ll<<NUM_INS) && ret < max_ret; i++){
      fixed_sum_t val = llabs(getInitedRow(row<<(D*NUM_OUTS))[int_expand_by_D(i)]); // this has an implicit /2  due to the types.
      if(val == 0) continue; // nothing to add

      shift_t max_shift = LEAD_1(max_ret)-1 - LEAD_1(val);
      shift_t shift_by = get_row_multeplicity(probes, row);
      if(shift_by > max_shift){  // ensure no overflow
        ret = max_ret;
      }else{
        ret += val << shift_by;
      }
    }
  })

  if(ret > max_ret) ret = max_ret;
  return ret;
}


// calculate the coefficient of the f(p) using evalCombination to get the individual coefficient of each combination of probes.
void min_failure_probability(double *retCoeffs, fixed_sum_t (*evalCombination)(prob_comb_t)){
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    retCoeffs[i] = 0.0;
  }

  ITERATE_OVER_PROBES(probes, coeffNum, {
    double mul = get_probes_multeplicity(probes);
    retCoeffs[coeffNum] += evalCombination(probes) / ((double) (1ll << (NUM_TOT_INS+1))) * mul;
  })
}


// output the info.
int main(){
  printf("program started\n");

  printf("calculating core...\n");
  calculateCore();
  printf("core calculated\n");


  double retCoeff[MAX_COEFF+1];

  min_failure_probability(retCoeff, rps_is);
  printf("coeffs of is: ");
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  min_failure_probability(retCoeff, rps_sum);
  printf("coeffs of sum: ");
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  return 0;
}
