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


#define INIT_STATUS__UNUSED  0
#define INIT_STATUS__ALLOCATED  1
#define INIT_STATUS__ONLYROW  2
#define INIT_STATUS__PROBE  3

typedef struct {
  int init_status;
  row_t index;
  fixed_sum_t onlyRow[NUM_NORND_COLS+1]; // 0~(NUM_NORND_COLS-1) rpc,  NUM_NORND_COLS rps
  fixed_sum_t probe[NUM_NORND_COLS+1];
  fixed_sum_t rpc_probe_min;
} elem_t;

static size_t storage_items;
static elem_t *storage;

static void __attribute__ ((constructor)) allocStorage(){
  storage = calloc(sizeof(elem_t), 1ll << CORRELATIONTABLE_STORAGE_BITS);
  if(storage == NULL) FAIL("Can't allocate storage for CorrelationTable!")
}

#define ROT(x, pos) (((x) << (pos)) | ((x) >> (-(pos) & 63)))
// must be pow2
#define HT_BLOCK 32

#if HT_BLOCK > 1ll << CORRELATIONTABLE_STORAGE_BITS
  #undef HT_BLOCK
  #define HT_BLOCK (1ll << CORRELATIONTABLE_STORAGE_BITS)
#endif

static inline hash_t get_hash(row_t index, hash_t counter){
  hash_t hash = 0;

  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    hash_t it = index.values[i];
    hash ^= (it * 11) ^ ROT(it, 17) ^ (ROT(it, 35) * (counter + 12));
    hash = (hash * 11) ^ ROT(hash, 17);
    hash = (hash * 11) ^ ROT(hash, 17);
  }

  return (hash >> (64-CORRELATIONTABLE_STORAGE_BITS)) ^ (hash & ((1ll << CORRELATIONTABLE_STORAGE_BITS)-1));
}

static uint64_t hash_requests;
static uint64_t hash_lookups;

static elem_t* get_row(row_t index){
  hash_requests++;
  hash_t counter = 0;
  while(1){
    hash_t hash  = get_hash(index, counter++);
    for(int i = 0; i < HT_BLOCK; i++){
      hash_t hash_it = (hash &~(HT_BLOCK-1)) | ((hash+i) & (HT_BLOCK-1));
      elem_t* it = &storage[ hash_it ];
      hash_lookups++;
      if(it->init_status == INIT_STATUS__UNUSED){
        storage_items++;
        it->index = index;
        it->init_status = INIT_STATUS__ALLOCATED;
        return it;
      }

      if(row_eq(it->index, index))
        return it;
    }
  }
}


/*
double correlationTable_dbg_storageFill(void){
  return ((double) storage_items) / (1ll << CORRELATIONTABLE_STORAGE_BITS);
}
double correlationTable_dbg_hashConflictRate(void){
  return ((double) hash_lookups - hash_requests) / hash_requests;
}
*/

double correlationTable_dbg_storageFill(void){
  return ((double) storage_items) / (1ll << CORRELATIONTABLE_STORAGE_BITS);
}
double correlationTable_dbg_storageHashConflictRate(void){
  return ((double) hash_lookups - hash_requests) / hash_requests;
}
double correlationTable_dbg_assocFill(void){
  return 0.0;
}
double correlationTable_dbg_assocHashConflictRate(void){
  return 0.0;
}




// -- correlationTable_row_tryInsertTransform


// with D=3, from  [0100] -> [000'111'000'000]. from the underlinying wire to all its shares.
static col_t intExpandByD(col_t val){
  if(val == 0) return 0;
  col_t bit = (1ll<<D)-1;
  col_t ret = 0;
  for(shift_t i = 0 ; i <= LEAD_1(val); i++){ // val==0 has been handled.
    ret |= (((val >> i) & 1) * bit) << i*D;
  }
  return ret;
}

static int maxShares(col_t value){
  int ret = 0;
  col_t mask = (1ll << D)-1;
  for(int i = 0; i < NUM_INS; i++){
    int v = __builtin_popcountll(value & (mask << (i*D)));
    ret = v > ret ? v : ret;
  }
  return ret;
}

// ret = min(1, \sum_{i \subseteq [I^{U^g}] : i \neq \emptyset } |W_{row, iD + [D]}|)
static fixed_sum_t get_rowRps(fixed_cell_t transform[NUM_NORND_COLS]){
  fixed_sum_t ret = 0;
  for(int i = 1; i < (1ll<<NUM_INS) && ret < MAX_FIXED_SUM; i++){
    ret += llabs(transform[intExpandByD(i)]);
  }
  return FIXED_SUM_NORMALIZE(ret);
}

// for I \subseteq [I^g] : maxShares(I) \leq t,  ret_I = min(1, \sum{i \subseteq [I^g] : i \backslash I \neq \emptyset} |W_{row, i}|)
static void get_rowRpc(fixed_cell_t transform[NUM_NORND_COLS], fixed_sum_t ret[NUM_NORND_COLS]){
  for(int ii = 0; ii < NUM_NORND_COLS; ii++){
    if(maxShares(ii) > T){
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

void correlationTable_row_insertTransform(row_t row, fixed_cell_t transform[NUM_NORND_COLS]){
  elem_t* r = get_row(row);

  if(r->init_status == INIT_STATUS__UNUSED) FAIL("correlationTable_row_insertTransform: must be allocated, not unused!")
  if(r->init_status == INIT_STATUS__ONLYROW) FAIL("correlationTable_row_insertTransform: must be allocated, not onlyrow!")
  if(r->init_status == INIT_STATUS__PROBE) FAIL("correlationTable_row_insertTransform: must be allocated, not probe!")
  if(r->init_status != INIT_STATUS__ALLOCATED) FAIL("correlationTable_row_insertTransform: must be allocated!")

  r->onlyRow[NUM_NORND_COLS] = get_rowRps(transform);
  get_rowRpc(transform, r->onlyRow);
  r->init_status = INIT_STATUS__ONLYROW;
}

// -- correlationTable calculate probe info


// used as an iterator to get the next sub probe combination.
static bool tryGetNextRow(row_t highestRow, row_t *curr){ // first is 0, return 0 for end
  if(row_eq(*curr, highestRow)) return 0;

  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    row_value_t curr_zeros = highestRow.values[i] ^ curr->values[i];  // curr_zeros has a 1 where curr has a meaningful 0
    if(curr_zeros == 0){
      curr->values[i] = 0;
      continue;
    }

    row_value_t lowest_0 = 1ll << TAIL_1(curr_zeros); // lowest meningful 0 of curr
    curr->values[i] &= ~(lowest_0-1); // remove anything below the lowest 0
    curr->values[i] |= lowest_0; // change the lowest 0 to 1.
    break;
  }
  return 1;
}


elem_t *caculate_probe_info(row_t rowParam);

// row->probe_field  = \sum_{sub \subseteq row} sub->onlyRow_field.     returns if success.
void set_single_info(elem_t * row, int index) {
  fixed_sum_t row_ors = row->onlyRow[index];
  fixed_sum_t *row_ps = &row->probe[index];
  *row_ps = row_ors == MAX_FIXED_SUM ? MAX_FIXED_SUM : 0;

  /* half rows ; get it from the existing 'probe_rps', those with the same combination except 1 bit changed 1->0. */
  row_t max_sub = row->index;  /* the max is used and not the first one to detect sum >= MAX_FIXED_SUM earlier */
  fixed_sum_t max_sub_rps_probe = 0;
  for(int j = 0; j < ROW_VALUES_SIZE && *row_ps < MAX_FIXED_SUM; j++){
    row_value_t row_it = row->index.values[j];
    if(row_it == 0) continue;

    for(shift_t i = TAIL_1(row_it); i <= LEAD_1(row_it) && *row_ps < MAX_FIXED_SUM; i++){
      row_value_t sub_it = row_it &~ (1ll << i);
      if(sub_it == row_it) continue; /* it's not a sub-probe */

      row_t sub = row->index;
      sub.values[j] = sub_it;
      elem_t* it = caculate_probe_info(sub);
      fixed_sum_t it_ps = it->probe[index];
      if(it_ps <= max_sub_rps_probe) continue; /* not the max */

      max_sub_rps_probe = it_ps; /* set new max */
      max_sub = sub;
      if(it_ps + row_ors >= MAX_FIXED_SUM){  /* this by itself saturated the maximum */
         *row_ps = MAX_FIXED_SUM;
      }
    }
  }
  *row_ps = FIXED_SUM_NORMALIZE(*row_ps + max_sub_rps_probe);

  /* other half rows ; get them one by one */
  row_t fixed = row_xor(max_sub, row->index); /* add the fixed 1 later, iterate over the combinations without it as it's easier. */
  row_t sub = row_first();
  if (*row_ps < MAX_FIXED_SUM) do{
    row_t actual = row_or(sub, fixed);
    if(!row_eq(actual, row->index)){
      elem_t *it = caculate_probe_info(actual);
      *row_ps += it->onlyRow[index];    /*max_sub = row->index ^ fixed = row->index &~ fixed  */
    }else{
      *row_ps += row_ors;
    }
  }while(*row_ps < MAX_FIXED_SUM && tryGetNextRow(max_sub, & sub)); /* break when it loops back, 0 is false. */
  *row_ps = FIXED_SUM_NORMALIZE(*row_ps);
}


elem_t *caculate_probe_info(row_t rowParam){
//printf("row: %llx", (int64_t) rowParam.values[0]);
  elem_t *row = get_row(rowParam);

  if(row->init_status == INIT_STATUS__PROBE) return row;
  if(row->init_status != INIT_STATUS__ONLYROW) FAIL("caculate_probe_info: row must be inited!")

//printf("2\n");

  set_single_info(row, NUM_NORND_COLS);

//printf("3\n");

  fixed_sum_t min = MAX_FIXED_SUM;
  for(int i = 0; i < NUM_NORND_COLS && min > 0; i++){
    if(maxShares(i) > T) continue;
    set_single_info(row, i);
    min = MIN(min, row->probe[i]);
  }
//printf("4\n");
  row->rpc_probe_min = min;

  row->init_status = INIT_STATUS__PROBE;
  return row;
}


// -- correlationTable_probe_getRP*


fixed_sum_t correlationTable_probe_getRPS(row_t row){
  return caculate_probe_info(row)->probe[NUM_NORND_COLS];
}

fixed_sum_t correlationTable_probe_getRPC(row_t row){
  return caculate_probe_info(row)->rpc_probe_min;
}

