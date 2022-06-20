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
typedef uint64_t hash_t;
typedef int32_t shift_t;
typedef uint64_t bitarray_t;
typedef int32_t fixed_cell_t;  // fixed point notation 0.NUM_TOT_INS
typedef uint64_t fixed_sum_t; // fixed point notation 1.(NUM_TOT_INS+1)
#define MAX_FIXED_SUM  (1ll << (NUM_TOT_INS+1))
#define FIXED_SUM_NORMALIZE(x)  ((x) > MAX_FIXED_SUM ? MAX_FIXED_SUM : (x))

// useful def
#define LEAD_1(x) (63 - __builtin_clzll((x)))
#define TAIL_1(x) LEAD_1((x)&-(x))


double binomial_d(int n, int k){
  if(k > n-k) return binomial_d(n, n-k);

  double ret = 1.0;
  for(int i = n-k+1; i <= n; i++)
    ret *= i;
  for(int i = 2; i <= k; i++)
    ret /= i;

  return ret;
}

row_t binomial_r(int n, int k){
  if(k > n-k) return binomial_r(n, n-k);

  row_t ret = 1;
  for(int i = n-k+1; i <= n; i++)
    ret *= i;
  for(int i = 2; i <= k; i++)
    ret /= i;

  return ret;
}



#define PLAIN_SIZE  ((NUM_COLS+63) / 64)

#define GET_PLAIN(arr, pos)  (((arr)[(pos) / 64] >> ((pos) % 64)) & 1)
#define SET_PLAIN(arr, pos, val)  {(arr)[(pos) / 64] ^= (GET_PLAIN(arr,pos) ^ (val))  << ((pos) % 64);}
#define SET_AS_XOR_PLAIN(to, f1, f2)  { for(col_t set_as_xor_plain__i = 0; set_as_xor_plain__i < PLAIN_SIZE; set_as_xor_plain__i++){ (to)[set_as_xor_plain__i] = (f1)[set_as_xor_plain__i] ^ (f2)[set_as_xor_plain__i];  } }




// -- hash table



typedef struct {
  row_t not_index; // negated for initialization. 0 -> all 1s -> >NUM_PROBES -> invalid.
  fixed_sum_t only_row;
  fixed_sum_t tot_sum;
} HT_elem_t;

typedef struct {
  HT_elem_t* table;
  shift_t bits;
} HT_t;

hash_t get_rows_group_size(shift_t coeff){
  hash_t ret = binomial_r(NUM_PROBES, coeff);
  if(ret != (hash_t) binomial_d(NUM_PROBES, coeff)){
    fprintf(stderr, "Rounding error!\n");
    fprintf(stderr, "Can't calculate number of combinations with %d probes.", (int) coeff);
    exit(1);
  }

  return ret;
}

HT_t new_HT(){
  hash_t size = 0;
  for(shift_t i = 0; i <= MAX_COEFF && i <= NUM_PROBES; i++){
    size += get_rows_group_size(i);
  }
  size = size * 120 / 100;

  HT_t ret;
  ret.bits = LEAD_1(size)+1;
  ret.table = calloc(1ll << ret.bits, sizeof(HT_elem_t));
  return ret;
}

#define ROT(x, pos) (((x) << (pos)) | ((x) >> (-(pos) & 63)))
#define HT_BLOCK 32

hash_t ht_count_values = 0;
inline hash_t get_hash(row_t index, shift_t bits, hash_t counter){
//printf("index = %lx, bits = %lx, counter = %lx, empty = %ld, full = %ld", (long) index, (long) bits, (long) counter, (long) (1ll<<bits) - ht_count_values, (long) ht_count_values);
  hash_t hash = (index * 11) ^ ROT(index, 17) ^ (ROT(index, 35) * (counter + 12));
//printf("h = %lx", hash);
  hash = (hash * 11) ^ ROT(hash, 17);
//printf("h = %lx", hash);
  hash = (hash * 11) ^ ROT(hash, 17);
//printf("h = %lx\n", hash);
  return (hash >> (64-bits)) ^ (hash & ((1ll << bits)-1));
}

HT_elem_t* get_value_HT(HT_t hashTable, row_t index){
  hash_t hash;

  hash_t counter = 0;
  while(1){
    hash  = get_hash(index, hashTable.bits, counter++);
    for(int i = 0; i < HT_BLOCK; i++, hash=(hash+1) & ((1ll << hashTable.bits)-1) ){
//printf("h = %lx\n", hash);
//printf("considering index = %lx, as hash = %lx\n", (long) index, (long) hash);
      if(hashTable.table[hash].not_index == ~index){
        return &hashTable.table[hash];
      }
      if(hashTable.table[hash].not_index == 0){
//printf("adding index = %lx, as hash = %lx\n", (long) index, (long) hash);
        ht_count_values++;
        hashTable.table[hash].not_index = ~index;
        if(~index == 0){
          fprintf(stderr, "How can a row_t be full of 1s?\n");
          exit(1);
        }
        return &hashTable.table[hash];
      }
    }
  }
}



//-- calculate info on the rows



// definition of the gadget, to be appended afterward.
void gadget(bool x[NUM_TOT_INS], bool ret[NUM_TOT_OUTS]);

void initCore(bitarray_t core[NUM_PROBES * PLAIN_SIZE]){
  bool x[NUM_TOT_INS];
  bool ret[NUM_TOT_OUTS];
  for(col_t i = 0; i < NUM_COLS; i++){
    for(shift_t j = 0; j < NUM_TOT_INS; j++){
      x[j] = (i >> j) & 1;
    }
    gadget(x, ret);
    for(shift_t j = 0; j < NUM_PROBES; j++){
      SET_PLAIN(core + j * PLAIN_SIZE, i, ret[j + NUM_OUTS*D])
    }
  }
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

fixed_sum_t get_row_half_sum(bitarray_t *row, row_t r){
  static const fixed_cell_t val[2] = {1, -1};

  fixed_cell_t transformed[NUM_COLS];
  for(col_t i = 0; i < NUM_COLS; i++){
    transformed[i] = val[GET_PLAIN(row, i)];
  }
  inPlace_transform(transformed, NUM_COLS);

  fixed_sum_t ret = 0;
  for(col_t i = 1; i < (1ll<<NUM_INS) && ret < MAX_FIXED_SUM; i++){
    ret += llabs(transformed[int_expand_by_D(i)]); // this has an implicit /2  due to the types.
  }
/*
if(ret != 0){
printf("half_sum of %lx is %lx\n", (long) r, (long) ret);
for(col_t j = 0; j < NUM_NORND_COLS; j++) printf("%ld; ", (long) transformed[j]);
printf("\n");
}
*/
  return FIXED_SUM_NORMALIZE(ret);
}

row_t get_first_row_popk(shift_t population){ // population must be > 0
  return (1ll << population)-1;
}

inline row_t get_next_row_popk(shift_t population, row_t curr){
  row_t ret = curr;
  ret += (1ll << TAIL_1(ret));
  ret |= (1ll << (population - __builtin_popcountll(ret)))-1;

  if(ret < curr) return 0; // looped back
  if(ret >= (1ll << NUM_PROBES)) return 0; // reached the end
  return ret;
}

// used as an iterator to get the next sub probe combination.
row_t getNextRow(row_t highestRow, row_t curr){ // first is 0, return 0 for end
  if(curr == highestRow) return 0;
  row_t lowest = 1ll << TAIL_1(curr ^ highestRow);  // sub ^ probes == 0 <-> sub == probes, which has been handled.
  return (curr & ~(lowest-1)) | lowest;
}

fixed_sum_t calculateTotSum(fixed_sum_t only_row, HT_t ht, row_t row){
  if(only_row == MAX_FIXED_SUM) return MAX_FIXED_SUM;

  // half rows
  row_t max_parent = 0;  // the max to detect sum >= MAX_FIXED_SUM earlier
  fixed_sum_t max_parent_tot_sum = 0;
  for(shift_t i = TAIL_1(row); i <= LEAD_1(row); i++){
    row_t parent = row &~(1ll<<i);
    if(parent == row) continue; // probe not present

    HT_elem_t* it = get_value_HT(ht, parent);
    if(it->tot_sum <= max_parent_tot_sum) continue; // not the max

    max_parent_tot_sum = it->tot_sum; // set new max
    max_parent = parent;
    if(max_parent_tot_sum + only_row >= MAX_FIXED_SUM) return MAX_FIXED_SUM; // this by itself saturated the maximum
  }
  fixed_sum_t ret = max_parent_tot_sum;

  // other half rows
  row_t fixed = max_parent ^ row; // add the fixed 1 later, iterate over the combinations without it as it's easier.
  row_t sub = 0;
  do{
    ret += get_value_HT(ht, fixed | sub)->only_row;
    if(ret >= MAX_FIXED_SUM) return MAX_FIXED_SUM;
  }while((sub = getNextRow(max_parent, sub))); // break when it loops back, 0 is false.
//printf("row = %lx, ret = %lx\n", (long) row, (long) ret);
  return ret;
}

int64_t getNextProbe(int64_t probe){ /// 0 is the first probe
  if(__builtin_popcountll(probe+1) <= MAX_COEFF) return probe+1;
  if(probe == 0) return MAX_COEFF != 0 ? 1 : 0;

  int64_t lowest = 1ll << TAIL_1(probe);  // probes == 0 has been handled.
  probe += lowest;
  probe &= ~(1ll << NUM_PROBES); // if overflows return 0 i.e. loop back.
  return probe;
}

HT_t calculateAll(){
  bitarray_t core[NUM_PROBES * PLAIN_SIZE];
  initCore(core);

  HT_t ret = new_HT();

  row_t row = 0;
  row_t prev = ~row;
  bitarray_t plain[PLAIN_SIZE] = {0};
  do{
//printf("row %lx\n", row);
    //either reset or reuse last result
    row_t xorWith;
    if(__builtin_popcountll(row) > __builtin_popcountll(row ^ prev)){
      xorWith = row ^ prev;
    }else{
      xorWith = row;
      for(col_t i = 0; i < PLAIN_SIZE; i++){
        plain[i] = 0;
      }
    }

    // get the plain result
    while(xorWith != 0){
      shift_t tail = TAIL_1(xorWith);
      SET_AS_XOR_PLAIN(plain, plain, core + tail * PLAIN_SIZE)
      xorWith ^= 1ll << tail;
    }
//printf("for done\n");

    // calculate the element of the hash table.
    HT_elem_t *e = get_value_HT(ret, row);
//printf("hash table done\n");
    e->only_row = get_row_half_sum(plain, row);
    e->tot_sum = calculateTotSum(e->only_row, ret, row);

    // next
    prev = row;
  }while((row = getNextProbe(row)));

  return ret;
}



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


#define ITERATE_OVER_ROWS_OF_PROBE(curr_comb, iterator, code)  { \
    row_t iterator = 0; \
    row_t iterateOverRowsOfProbe_highestRow = getHighestRow((curr_comb)); \
    do{ \
      code \
    }while((iterator = getNextRow(iterateOverRowsOfProbe_highestRow, iterator))); \
  }

// return the logaritm, how much it needs to be shifted
shift_t get_row_multeplicity(prob_comb_t curr_comb){
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

double get_probes_multeplicity(prob_comb_t curr_comb){
  double ret = 1.0;
  for(shift_t i = 0; i < NUM_PROBES; i++){
    ret *= binomial_d(multeplicity[i], curr_comb[i]);
  }
  return ret;
}






// calculate with the 'is' formula
fixed_sum_t rps_is(HT_t ht, prob_comb_t probes){ // returns with fixed point notation.
  return get_value_HT(ht, getHighestRow(probes))->tot_sum != 0 ? MAX_FIXED_SUM : 0;
}

// calculate with the 'sum' formula
fixed_sum_t rps_sum(HT_t ht, prob_comb_t probes){ // returns with fixed point notation.
  shift_t shift_by = get_row_multeplicity(probes);
  fixed_sum_t ret = get_value_HT(ht, getHighestRow(probes))->tot_sum;

  if(ret == 0) return 0; // to avoid bug in the next line if  MAX_FIXED_SUM >> shift_by == 0
  if(ret >= (MAX_FIXED_SUM >> shift_by) ) return MAX_FIXED_SUM; // nearly the same as asking if  (ret << shift_by) >= MAX_FIXED_SUM

  return ret << shift_by;
}


// calculate the coefficient of the f(p) using evalCombination to get the individual coefficient of each combination of probes.
void min_failure_probability(HT_t ht, double *retCoeffs, fixed_sum_t (*evalCombination)(HT_t, prob_comb_t)){
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    retCoeffs[i] = 0.0;
  }

  ITERATE_OVER_PROBES(probes, coeffNum, {
    double mul = get_probes_multeplicity(probes);
    retCoeffs[coeffNum] += evalCombination(ht, probes) / ((double) (1ll << (NUM_TOT_INS+1))) * mul;
  })
}


// output the info.
int main(){
  printf("program started\n");

  printf("calculating matrix info...\n");
  HT_t ht = calculateAll();
  printf("info calculated\n");


  double retCoeff[MAX_COEFF+1];

  min_failure_probability(ht, retCoeff, rps_is);
  printf("coeffs of is: ");
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  min_failure_probability(ht, retCoeff, rps_sum);
  printf("coeffs of sum: ");
  for(shift_t i = 0; i <= MAX_COEFF; i++){
    printf(" %f", retCoeff[i]);
  }
  printf("\n");

  return 0;
}
