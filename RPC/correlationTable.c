#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "types.h"
#include "correlationTable.h"




#if D * NUM_OUTS > 64
  #error "Current algprithm doesn't support D * NUM_OUTS > 64"
#endif
#if D * NUM_INS > 64
  #error "Current typing doesn't support D * NUM_INS > 64"
#endif



typedef uint64_t hash_t;
#define MAX_FIXED_SUM  (1ll << (NUM_TOT_INS+1))
#define FIXED_SUM_NORMALIZE(x)  ((x) > MAX_FIXED_SUM ? MAX_FIXED_SUM : (x))

// useful def
#define LEAD_1(x) (63 - __builtin_clzll((x)))
#define TAIL_1(x) LEAD_1((x)&-(x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define FAIL(...)  { fprintf(stderr, __VA_ARGS__); exit(1); }


// -- storage


typedef struct {
  hash_t base_hash; // 0 for unused
  fixed_cell_t transform[NUM_NORND_COLS];
  fixed_sum_t onlyRow_rps;
  fixed_sum_t onlyRow_rpc[NUM_NORND_COLS];
  bool onlyRow_rpc_is[NUM_NORND_COLS];
} elem_t;

static size_t storage_items;
static elem_t *storage;

static void __attribute__ ((constructor)) allocStorage(){
  storage = calloc(sizeof(elem_t), 1ll << CORRELATIONTABLE_STORAGE_BITS);
  if(storage == NULL) FAIL("Can't allocate storage for CorrelationTable!")
}

#define ROT(x, pos) (((x) << (pos)) | ((x) >> (-(pos) & 63)))
// must be pow2
#define STORAGE_HT_BLOCK 32

#if STORAGE_HT_BLOCK > 1ll << CORRELATIONTABLE_STORAGE_BITS
  #undef STORAGE_HT_BLOCK
  #define STORAGE_HT_BLOCK (1ll << CORRELATIONTABLE_STORAGE_BITS)
#endif

static inline hash_t get_hash_content(fixed_cell_t transform[NUM_NORND_COLS], hash_t counter, hash_t* base_hash){
  hash_t hash = 0;

  for(int i = 0; i < NUM_NORND_COLS; i++){
    hash_t it = transform[i] + counter;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);

//    hash ^= (it * 11) ^ ROT(it, 17) ^ (ROT(it, 35) * (counter + 12));
//    hash = (hash * 11) ^ ROT(hash, 17);
//    hash = (hash * 11) ^ ROT(hash, 17);
  }
  hash = (hash * 0xB0000B) ^ ROT(hash, 17);

  *base_hash = hash;
  return (hash >> (64-CORRELATIONTABLE_STORAGE_BITS)) ^ (hash & ((1ll << CORRELATIONTABLE_STORAGE_BITS)-1));
}

static uint64_t storage_hash_requests;
static uint64_t storage_hash_lookups;

static elem_t* get_row_from_transform(fixed_cell_t transform[NUM_NORND_COLS], void (*init)(elem_t*)){
  storage_hash_requests++;
  hash_t counter = 0;
  while(1){
    hash_t base_hash;
    hash_t hash = get_hash_content(transform, counter++, & base_hash);
    if(base_hash == 0)
      base_hash++;

    for(int i = 0; i < STORAGE_HT_BLOCK; i++){
      hash_t hash_it = (hash &~(STORAGE_HT_BLOCK-1)) | ((hash+i) & (STORAGE_HT_BLOCK-1));
      elem_t* it = &storage[ hash_it ];
      storage_hash_lookups++;
      if(it->base_hash == 0){
        storage_items++;
        for(int i = 0; i < NUM_NORND_COLS; i++){
          it->transform[i] = transform[i];
        }
        it->base_hash = base_hash;
        init(it);
        return it;
      }

      if(it->base_hash != base_hash) continue; // speed up
      for(int i = 0; i < NUM_NORND_COLS; i++)
        if(it->transform[i] != transform[i])
          continue;

      // it->transform == transform
      return it;
    }
  }
}



typedef struct {
  row_t index;
  elem_t* row;
  bool probe_rpc_is;
  fixed_sum_t probe_rpc;
  fixed_sum_t probe_rps;
} assoc_t;

static size_t assoc_items;
static assoc_t *assoc;

static void __attribute__ ((constructor)) allocAssoc(){
  assoc = calloc(sizeof(assoc_t), 1ll << ROWS_USED_BITS);
  if(assoc == NULL) FAIL("Can't allocate assoc for CorrelationTable!")
}

// must be pow2
#define ASSOC_HT_BLOCK 32

#if ASSOC_HT_BLOCK > 1ll << ROWS_USED_BITS
  #undef ASSOC_HT_BLOCK
  #define ASSOC_HT_BLOCK (1ll << ROWS_USED_BITS)
#endif

static inline hash_t get_hash_index(row_t index, hash_t counter){
  hash_t hash = 0;

  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    hash_t it = index.values[i] + counter;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  }

  return (hash >> (64-ROWS_USED_BITS)) ^ (hash & ((1ll << ROWS_USED_BITS)-1));
}

static uint64_t assoc_hash_requests;
static uint64_t assoc_hash_lookups;

static assoc_t* new_row(row_t index, fixed_cell_t transform[NUM_NORND_COLS], void (*init)(elem_t*)){
  assoc_hash_requests++;
  hash_t counter = 0;
  while(1){
    hash_t hash = get_hash_index(index, counter++);
    for(int i = 0; i < ASSOC_HT_BLOCK; i++){
      hash_t hash_it = (hash &~(ASSOC_HT_BLOCK-1)) | ((hash+i) & (ASSOC_HT_BLOCK-1));
      assoc_t *it = &assoc[ hash_it ];
      assoc_hash_lookups++;
      if(it->row == NULL){
        assoc_items++;
        it->index = index;
        it->row = get_row_from_transform(transform, init);
        it->probe_rpc_is = 2;
        it->probe_rpc = MAX_FIXED_SUM+1;
        it->probe_rps = MAX_FIXED_SUM+1;
        return it;
      }

      if(row_eq(it->index, index)) FAIL("get_row_new: The row is alrady in!")
    }
  }
}

// -----

static assoc_t* get_row(row_t index){
  assoc_hash_requests++;
  hash_t counter = 0;
  while(1){
    hash_t hash  = get_hash_index(index, counter++);
    for(int i = 0; i < ASSOC_HT_BLOCK; i++){
      hash_t hash_it = (hash &~(ASSOC_HT_BLOCK-1)) | ((hash+i) & (ASSOC_HT_BLOCK-1));
      assoc_t *it = & assoc[ hash_it ];
      assoc_hash_lookups++;
      if(it->row == NULL) FAIL("get_row: missing!")

      if(row_eq(it->index, index))
        return it;
    }
  }
}

double correlationTable_dbg_storageFill(void){
  return ((double) storage_items) / (1ll << CORRELATIONTABLE_STORAGE_BITS);
}
double correlationTable_dbg_storageHashConflictRate(void){
  return ((double) storage_hash_lookups - storage_hash_requests) / storage_hash_requests;
}
double correlationTable_dbg_assocFill(void){
  return ((double) assoc_items) / (1ll << ROWS_USED_BITS);
}
double correlationTable_dbg_assocHashConflictRate(void){
  return ((double) assoc_hash_lookups - assoc_hash_requests) / assoc_hash_requests;
}



// -- correlationTable_row_tryInsertTransform


// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
static col_t intExpandByD(col_t val){
  if(val == 0) return 0;
  col_t bit = (1ll<<D)-1;
  col_t ret = 0;
  for(shift_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << (i*D);
  }
  return ret;
}

static int maxShares__i(col_t value, int num_wires){
  int ret = 0;
  col_t mask = (1ll << D)-1;
  for(int i = 0; i < num_wires; i++){
    int v = __builtin_popcountll(value & (mask << (i*D)));
    ret = v > ret ? v : ret;
  }
  return ret;
}

static int maxShares_in(col_t value){ return maxShares__i(value, NUM_INS); }
//static int maxShares_out(col_t value){ return maxShares__i(value, NUM_OUTS); }

// ret = min(1, \sum_{i \subseteq [I^{U^g}] : i \neq \emptyset } |W_{row, iD + [D]}|)
static fixed_sum_t get_rowRps(fixed_cell_t transform[NUM_NORND_COLS]){
  fixed_sum_t ret = 0;
  for(int i = 1; i < (1ll<<NUM_INS) && ret < MAX_FIXED_SUM; i++){
    ret += llabs(transform[intExpandByD(i)]);
  }
  return FIXED_SUM_NORMALIZE(ret);
}

// for I \subseteq [I^g] : maxShares(I) \leq t,  ret_I = is(\exist{i \subseteq [I^g] : i \backslash I \neq \emptyset} W_{row, i} \neq 0)
static void get_rowRpc_is(fixed_cell_t transform[NUM_NORND_COLS], bool ret[NUM_NORND_COLS]){
  for(int ii = 0; ii < NUM_NORND_COLS; ii++){
    if(maxShares_in(ii) > T){
       ret[ii] = 1;
       continue;
    }
    ret[ii] = 0;
    for(int i = 0; i < NUM_NORND_COLS && !ret[ii]; i++){
      if((i &~ ii) != 0 && transform[i] != 0)
        ret[ii] = 1;
    }
  }
}

// for I \subseteq [I^g] : maxShares(I) \leq t,  ret_I = min(1, \sum{i \subseteq [I^g] : i \backslash I \neq \emptyset} |W_{row, i}|)
static void get_rowRpc(fixed_cell_t transform[NUM_NORND_COLS], fixed_sum_t ret[NUM_NORND_COLS]){
  for(int ii = 0; ii < NUM_NORND_COLS; ii++){
    if(maxShares_in(ii) > T){
       ret[ii] = MAX_FIXED_SUM;
       continue;
    }
    ret[ii] = 0;
    for(int i = 0; i < NUM_NORND_COLS && ret[ii] < MAX_FIXED_SUM; i++){
      if((i &~ ii) != 0){
        ret[ii] += llabs(transform[i]);
      }
    }
    ret[ii] = FIXED_SUM_NORMALIZE(ret[ii]);
  }
}

static void row_elem_init(elem_t* r){
  r->onlyRow_rps = get_rowRps(r->transform);
  get_rowRpc(r->transform, r->onlyRow_rpc);
  get_rowRpc_is(r->transform, r->onlyRow_rpc_is);
}

void correlationTable_row_insertTransform(row_t row, fixed_cell_t transform[NUM_NORND_COLS]){
  new_row(row, transform, row_elem_init);
}

// -- correlationTable calculate probe info


// used as an iterator to get the next sub probe combination.
static bool tryGetNextRow(row_t highestRow, row_t *curr){ // first is 0, return 0 for end
  if(row_eq(*curr, highestRow)) return 0;

  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    row_value_t curr_zeros = highestRow.values[i] ^ curr->values[i];  // curr_zeros has a 1 where curr has a meaningful 0
    if(curr_zeros == 0){
      curr->values[i] = 0;
      continue; // carry
    }

    row_value_t lowest_0 = 1ll << TAIL_1(curr_zeros); // lowest meningful 0 of curr
    curr->values[i] &= ~(lowest_0-1); // remove anything below the lowest 0
    curr->values[i] |= lowest_0; // change the lowest 0 to 1.
    break;
  }
  return 1;
}



// -- correlationTable_probe_getRPS


fixed_sum_t correlationTable_probe_getRPS(row_t row_index){
  assoc_t *row = get_row(row_index);
  if(row->probe_rps <= MAX_FIXED_SUM) return row->probe_rps; // if already calculated

  fixed_sum_t onlyRow = row->row->onlyRow_rps;
  if(onlyRow == MAX_FIXED_SUM){
    row->probe_rps = MAX_FIXED_SUM;
    return MAX_FIXED_SUM;
  }

  /* check if any of the direct sub-probes has reached the max, if so then this is too */
  for(int j = 0; j < ROW_VALUES_SIZE; j++){
    row_value_t row_it = row_index.values[j];
    if(row_it == 0) continue;

    for(shift_t i = TAIL_1(row_it); i <= LEAD_1(row_it); i++){
      row_value_t sub_it = row_it &~ (1ll << i);
      if(sub_it == row_it) continue; /* it's not a sub-probe */
      row_t sub = row_index;
      sub.values[j] = sub_it;

      if(correlationTable_probe_getRPS(sub) + onlyRow >= MAX_FIXED_SUM){
        row->probe_rps = MAX_FIXED_SUM;
        return MAX_FIXED_SUM;
      }
    }
  }

  /* calculate them by summing the rows */
  row->probe_rps = 0;
  row_t sub = row_first();
  do{
    row->probe_rps += get_row(sub)->row->onlyRow_rps;
  }while(row->probe_rps < MAX_FIXED_SUM && tryGetNextRow(row_index, & sub)); /* break when it loops back, 0 is false. */
  row->probe_rps = FIXED_SUM_NORMALIZE(row->probe_rps);
  return row->probe_rps;
}


// -- correlationTable_probe_getRPC


static fixed_sum_t rpc_row_min(fixed_sum_t* row){
  fixed_sum_t min = MAX_FIXED_SUM;
  for(int i = 0; i < NUM_NORND_COLS && min != 0; i++)
    if(maxShares_in(i) <= T)
      min = MIN(min, row[i]);
  return min;
}


fixed_sum_t correlationTable_probe_getRPC(row_t row_index){
  assoc_t *row = get_row(row_index);
  if(row->probe_rpc <= MAX_FIXED_SUM) return row->probe_rpc; // if already calculated

//#if 1==0
  fixed_sum_t* onlyRow = row->row->onlyRow_rpc;
  fixed_sum_t onlyRow_min = rpc_row_min(onlyRow);

  if(onlyRow_min == MAX_FIXED_SUM){
    row->probe_rpc = MAX_FIXED_SUM;
    return MAX_FIXED_SUM;
  }


  /* check if any of the direct sub-probes has reached the max, if so then this is too */
  for(int j = 0; j < ROW_VALUES_SIZE; j++){
    row_value_t row_it = row_index.values[j];
    if(row_it == 0) continue;

    for(shift_t i = TAIL_1(row_it); i <= LEAD_1(row_it); i++){
      row_value_t sub_it = row_it &~ (1ll << i);
      if(sub_it == row_it) continue; /* it's not a sub-probe */
      row_t sub = row_index;
      sub.values[j] = sub_it;

      if(correlationTable_probe_getRPC(sub) + onlyRow_min >= MAX_FIXED_SUM){
        row->probe_rpc = MAX_FIXED_SUM;
        return MAX_FIXED_SUM;
      }
    }
  }
//#endif

  /* calculate them by summing the rows */
  fixed_sum_t probe[NUM_NORND_COLS] = {0};
  row_t sub = row_first();
  do{
    fixed_sum_t *it = get_row(sub)->row->onlyRow_rpc;
    for(int i = 0; i < NUM_NORND_COLS; i++)
      if(maxShares_in(i) <= T)
        probe[i] = FIXED_SUM_NORMALIZE(probe[i] + it[i]);

    row->probe_rpc = rpc_row_min(probe);
  }while(row->probe_rpc < MAX_FIXED_SUM && tryGetNextRow(row_index, & sub)); /* break when it loops back, 0 is false. */

  return row->probe_rpc;
}


// -- correlationTable_probe_getRPC_is


bool correlationTable_probe_getRPC_is(row_t row_index){
  assoc_t *row = get_row(row_index);
  if(row->probe_rpc_is != 2) return row->probe_rpc_is; // if already calculated

  /* calculate them by checking all the rows */
  for(int i = 0; i < NUM_NORND_COLS; i++){
    if(maxShares_in(i) > T) continue;


    row_t sub = row_first();
    bool it = 0;
    do{
      it = get_row(sub)->row->onlyRow_rpc_is[i];
    }while(!it && tryGetNextRow(row_index, & sub)); /* break when it loops back, 0 is false. */  // is an exists, so terminate when it finds anything at 1.

    if(!it) return 0; // all needs to be 1. if any is not, the result is not.
  }
  return 1;
}
