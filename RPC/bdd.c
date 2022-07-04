#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


#include "bdd.h"


#ifndef NUM_TOT_INS__LOG2
  #define NUM_TOT_INS__LOG2  8
#endif

#if NUM_INS * D > (1ll << NUM_TOT_INS__LOG2)
  #error "increase NUM_TOT_INS__LOG2"
#endif

#define FAIL(...)  { fprintf(stderr, __VA_ARGS__); exit(1); }






// -- storageless


#define BDD_FLAGS 2
#define BDD_FLAG__NOT (((uint64_t) 1) << 62)
#define BDD_FLAG__NODE (((uint64_t) 1) << 63)

#define bdd_op_neg(x)  ((x) ^ BDD_FLAG__NOT)
#define bdd_is_negated(x)  (((x) & BDD_FLAG__NOT) != 0)
#define bdd_is_node(x)  (((x) & BDD_FLAG__NODE) != 0)

#define bdd_get_input(x) (((x) >> (64 - BDD_FLAGS - NUM_TOT_INS__LOG2)) & ((1ll << NUM_TOT_INS__LOG2)-1))
#define bdd_place_input(in)  (((uint64_t) (in)) << (64 - BDD_FLAGS - NUM_TOT_INS__LOG2))

#define BDD_TRUE   0
#define BDD_FALSE  BDD_FLAG__NOT



bdd_t bdd_op_not(bdd_t val){
  return bdd_op_neg(val);
}
bdd_t bdd_val_const(bool val){
  return val ? BDD_TRUE : BDD_FALSE;
}



// -- storage

typedef struct{ bdd_t v[2]; } bdd_node_t;
static size_t stored_items;
static bdd_node_t *storage;

static void __attribute__ ((constructor)) allocStorage(){
  storage = calloc(sizeof(bdd_node_t), 1ll << BDD_STORAGE_BITS);
  if(storage == NULL) FAIL("Can't allocate the storage for BDDs!")
}



typedef uint64_t hash_t;

#define ROT(x, pos) (((x) << (pos)) | ((x) >> (-(pos) & 63)))
// must be a pow of 2
#define HT_BLOCK  32

static inline hash_t bdd_new_node__h(bdd_t sub0, bdd_t sub1, hash_t counter){
  hash_t hash0 = (sub0 * 11) ^ ROT(sub0, 17) ^ (ROT(sub0, 35) * (counter + 12));
  hash_t hash1 = (sub1 * 13) ^ ROT(sub1, 23) ^ (ROT(sub1, 39) * (counter + 13));

  hash0 = (hash0 * 11) ^ ROT(hash0, 17);
  hash1 = (hash1 * 13) ^ ROT(hash1, 23);
  hash0 = (hash0 * 11) ^ ROT(hash0, 17);
  hash1 = (hash1 * 13) ^ ROT(hash1, 23);

  hash0 = (hash0 * 11) ^ ROT(hash1, 17);
  hash1 = (hash1 * 13) ^ ROT(hash0, 23);
  hash0 = (hash0 * 11) ^ ROT(hash1, 17);
  hash1 = (hash1 * 13) ^ ROT(hash0, 23);

  return ((hash0 ^ hash1) >> (64-BDD_STORAGE_BITS)) ^ ((hash0 ^ hash1) & ((1ll << BDD_STORAGE_BITS)-1));
}


static uint64_t hash_lookups;
static uint64_t hash_requests;
static inline bdd_t bdd_new_node(bdd_t sub0, bdd_t sub1, shift_t input){   // add GC for when nearly empty.
  if(sub0 == sub1) return sub0; // simplify
  if(bdd_is_negated(sub0)) return bdd_op_neg(bdd_new_node(bdd_op_neg(sub0), bdd_op_neg(sub1), input)); // ensure it's unique: have the sub0 always positive

  hash_requests++;
  for(hash_t counter = 0; 1; counter++){
    hash_t hash  = bdd_new_node__h(sub0, sub1, counter);
    for(int i = 0; i < HT_BLOCK; i++){ // locality: search within the block
      hash_t hash_it = (hash & ~(HT_BLOCK-1)) | ((hash+i) & (HT_BLOCK-1));
      bdd_node_t* it = & storage[hash_it];
      hash_lookups++;
      if(it->v[0] == it->v[1]){ // a real node's children can't be the same or it'd be simplified -> must be an unused location
        stored_items++;
        it->v[0] = sub0;
        it->v[1] = sub1;
        return hash_it | BDD_FLAG__NODE | bdd_place_input(input);
      }
      if(it->v[0] == sub0 && it->v[1] == sub1){
        return hash_it | BDD_FLAG__NODE | bdd_place_input(input);
      }
    }
  }
}

#define BDD_NODE__ADDR_MASK ((((uint64_t) 1) << (64 - BDD_FLAGS - NUM_TOT_INS__LOG2))-1)
#define bdd_get_node(x)  (storage[(x) & BDD_NODE__ADDR_MASK])

double bdd_dbg_storageFill(void){
  return ((double) stored_items) / (1ll << BDD_STORAGE_BITS);
}

double bdd_dbg_hashConflictRate(void){
  return ((double) hash_lookups - hash_requests) / hash_requests;
}


// -- bdd_dbg_print

static void bdd_dbg_print__i(bdd_t val, int padding){
  if(!bdd_is_node(val)){
    for(int i = 0; i < padding; i++) printf(" ");
    printf(val == BDD_TRUE? "TRUE\n" : "FALSE\n");
    return;
  }

  for(int i = 0; i < padding; i++) printf(" ");
  printf("%s{ %lld:\n", bdd_is_negated(val) ? "!" : "", bdd_get_input(val));

  bdd_node_t a = bdd_get_node(val);
  bdd_dbg_print__i(a.v[0], padding+2);
  bdd_dbg_print__i(a.v[1], padding+2);

  for(int i = 0; i < padding; i++) printf(" ");
  printf("}\n");
}

void bdd_dbg_print(bdd_t val){
  bdd_dbg_print__i(val, 0);
}

// -- bdd_val_single


bdd_t bdd_val_single(shift_t inputBit){
  return bdd_new_node(BDD_FALSE, BDD_TRUE, inputBit);
}


// -- cache

#define CACHE_OPTYPE__UNUSED  0
#define CACHE_OPTYPE__XOR  1
#define CACHE_OPTYPE__AND  2
#define CACHE_OPTYPE__SUM  4

static inline hash_t cache__h(uint64_t opType, bdd_t sub0, bdd_t sub1){
  hash_t hash0 = (sub0 * 11) ^ ROT(sub0, 17) ^ (ROT(sub0, 35) * 12);
  hash_t hash1 = (sub1 * 13) ^ ROT(sub1, 23) ^ (ROT(sub1, 39) * 13);

  hash0 = (hash0 * 11) ^ ROT(hash0, 17) ^ opType;
  hash1 = (hash1 * 13) ^ ROT(hash1, 23);
  hash0 = (hash0 * 11) ^ ROT(hash0, 17);
  hash1 = (hash1 * 13) ^ ROT(hash1, 23);

  hash0 = (hash0 * 11) ^ ROT(hash1, 17);
  hash1 = (hash1 * 13) ^ ROT(hash0, 23);
  hash0 = (hash0 * 11) ^ ROT(hash1, 17);
  hash1 = (hash1 * 13) ^ ROT(hash0, 23);

  hash_t h0 = (hash0 ^ hash1) >> (64-BDD_CACHE_BITS);
  hash_t h1 = (hash0 ^ hash1) & ((1ll << BDD_CACHE_BITS)-1);
  return (h0 ^ h1) & ~(BDD_CACHE_WAYS-1);
}

typedef struct{ uint64_t opType; bdd_t sub[2]; uint64_t value; } cache_elem_t;

static cache_elem_t cache[1ll << BDD_CACHE_BITS];
static uint64_t cache_items;

static inline bool cache_contains__index(uint64_t opType, bdd_t sub0, bdd_t sub1, int *ret_index){
  hash_t hash = cache__h(opType, sub0, sub1);
  for(int i = 0; i < BDD_CACHE_WAYS; i++){ // associative cache
    hash_t hash_it = hash | i;
    cache_elem_t* it = & cache[hash_it];
    if(it->opType == CACHE_OPTYPE__UNUSED){
      *ret_index = i;
      return 0;
    }

    if(it->opType == opType && it->sub[0] == sub0 && it->sub[1] == sub1){
      *ret_index = i;
      return 1;
    }
  }
  *ret_index = BDD_CACHE_WAYS;
  return 0;
}


static inline bool cache_contains(uint64_t opType, bdd_t sub0, bdd_t sub1){
  int ignore;
  return cache_contains__index(opType, sub0, sub1, &ignore);
}

static uint64_t cache_gos_movements;
static uint64_t cache_gos_requests;
static inline uint64_t cache_get_or_set(uint64_t opType, bdd_t sub0, bdd_t sub1, uint64_t value_if_missing){
  cache_gos_requests++;

  hash_t hash = cache__h(opType, sub0, sub1);

  int i;
  cache_elem_t to_add;
  if(cache_contains__index(opType, sub0, sub1, & i)) //get index
    to_add = cache[hash | i];
  else{
    cache_items++;
    to_add = (cache_elem_t){ opType, {sub0, sub1},  value_if_missing};

    // if full, remove the least accessed one.
    if(i == BDD_CACHE_WAYS) i--;
  }

  // move to create a slot for the last accessed one
  cache_gos_movements += i;
  for(;i > 0; i--)
    cache[hash | i] = cache[hash | (i-1)];
  cache[hash] = to_add;

  return to_add.value;
}

double bdd_dbg_cacheTurnover(void){
  return ((double) cache_items) / (1ll << BDD_CACHE_BITS);
}

double bdd_dbg_cacheMovementRate(void){
  return ((double) cache_gos_movements) / cache_gos_requests / (BDD_CACHE_WAYS-1);
}


// -- bdd_op_and


static bdd_t bdd_op_and__uncached(bdd_t val0, bdd_t val1){
  bdd_t val[2] = { val0, val1 };

  for(int i = 0; i < 2; i++){
    if(!bdd_is_node(val[i])){
      if(val[i] == BDD_FALSE) return BDD_FALSE;
      else return val[1-i]; // return the other
    }
  }

  bdd_node_t a[2] = { bdd_get_node(val0), bdd_get_node(val1) };
  shift_t i[2] = { bdd_get_input(val0), bdd_get_input(val1) };

  int min = i[0] <= i[1] ? 0 : 1;  // parameter with the minimal input, minimal -> first.
  int max = 1-min; // parameter with the other input.

  if(bdd_is_negated(val[min])){
    a[min].v[0] = bdd_op_neg(a[min].v[0]);
    a[min].v[1] = bdd_op_neg(a[min].v[1]);
  }
  if(i[min] == i[max] && bdd_is_negated(val[max])){
    a[max].v[0] = bdd_op_neg(a[max].v[0]);
    a[max].v[1] = bdd_op_neg(a[max].v[1]);
  }

  bdd_t sub[2];
  if(i[min] == i[max]){
    sub[0] = bdd_op_and(a[0].v[0], a[1].v[0]); // sub 0, v[0]
    sub[1] = bdd_op_and(a[0].v[1], a[1].v[1]);
  }else{
    sub[0] = bdd_op_and(a[min].v[0], val[max]);  // sub 0, v[0]
    sub[1] = bdd_op_and(a[min].v[1], val[max]); // only open the one with the first input, leave the other unchanged.
  }

  return bdd_new_node(sub[0], sub[1], i[min]);
}

bdd_t bdd_op_and(bdd_t val0, bdd_t val1){
  uint64_t value = 0;
  if(!cache_contains(CACHE_OPTYPE__AND, val0, val1))
    value = bdd_op_and__uncached(val0, val1);
  return cache_get_or_set(CACHE_OPTYPE__AND, val0, val1, value);
}


// -- bdd_op_xor

static bdd_t bdd_op_xor__uncached(bdd_t val0, bdd_t val1){
  bdd_t val[2] = { val0, val1 };

  // handle constants
  for(int i = 0; i < 2; i++){
    if(!bdd_is_node(val[i])){
      if(val[i] == BDD_FALSE) return val[1-i]; // return the other
      else return bdd_op_neg(val[1-i]); // xor true = not
    }
  }

  // handle negation
  int count_neg = 0;
  for(int i = 0; i < 2; i++){
    if(bdd_is_negated(val[i])){
      count_neg += 1;
      val[i] = bdd_op_neg(val[i]);
    }
  }
  if(count_neg == 1) return bdd_op_neg(bdd_op_xor(val[0], val[1]));

  bdd_node_t a[2] = { bdd_get_node(val[0]), bdd_get_node(val[1]) };
  shift_t i[2] = { bdd_get_input(val[0]), bdd_get_input(val[1]) };

  int min = i[0] < i[1] ? 0 : 1;  // parameter with the minimal input, minimal -> first.
  int max = 1-min; // parameter with the other input.

  bdd_t sub[2];
  if(i[min] == i[max]){
    sub[0] = bdd_op_xor(a[0].v[0], a[1].v[0]); // sub 0, v[0]
    sub[1] = bdd_op_xor(a[0].v[1], a[1].v[1]);
  }else{
    sub[0] = bdd_op_xor(a[min].v[0], val[max]);  // sub 0, v[0]
    sub[1] = bdd_op_xor(a[min].v[1], val[max]); // only open the one with the first input, leave the other unchanged.
  }

  return bdd_new_node(sub[0], sub[1], i[min]);
}

bdd_t bdd_op_xor(bdd_t val0, bdd_t val1){
  uint64_t value = 0;
  if(!cache_contains(CACHE_OPTYPE__XOR, val0, val1))
    value = bdd_op_xor__uncached(val0, val1);
  return cache_get_or_set(CACHE_OPTYPE__XOR, val0, val1, value);
}


// -- bdd_op_getTransformWithoutRnd


static fixed_cell_t bdd_op_getSumRNDs__sum(bdd_t val){ // do the sum recursively
  if(bdd_is_negated(val)) return -bdd_op_getSumRNDs__sum(bdd_op_neg(val));
  if(!bdd_is_node(val)) return -1ll << NUM_RANDOMS;

  uint64_t value = 0;
  if(!cache_contains(CACHE_OPTYPE__SUM, val, val)){
    bdd_node_t a = bdd_get_node(val);
    value = (bdd_op_getSumRNDs__sum(a.v[0]) + bdd_op_getSumRNDs__sum(a.v[1]))/2;
  }
  return cache_get_or_set(CACHE_OPTYPE__SUM, val, val, value);
}


static void bdd_op_getSumRNDs__i(bdd_t val, shift_t ins_missing, bool is_pos, fixed_cell_t *ret){ // recursively travel the BDD until you get to the randoms
  if(ins_missing == 0){
    if(!is_pos) val = bdd_op_neg(val);
    *ret = bdd_op_getSumRNDs__sum(val);
    return;
  }

  if(bdd_is_negated(val)){
    val = bdd_op_neg(val);
    is_pos = !is_pos;
  }

  bdd_node_t a;
  if(!bdd_is_node(val)) {
    a.v[0] = a.v[1] = val;
  } else if(bdd_get_input(val) != NUM_INS*D-ins_missing){ // the two a.v are the same.
    a.v[0] = a.v[1] = val;
  } else {
    a = bdd_get_node(val);
  }

  bdd_op_getSumRNDs__i(a.v[0], ins_missing-1, is_pos, ret);
  bdd_op_getSumRNDs__i(a.v[1], ins_missing-1, is_pos, ret + (1ll << (NUM_INS*D-ins_missing)));
}

// get for each input, the sum of the values, with true=1, false=-1.
static void bdd_op_getSumRNDs(bdd_t val, fixed_cell_t ret[NUM_NORND_COLS]){
  bdd_op_getSumRNDs__i(val, NUM_INS*D, 1, ret);
}

// Fast Walsh-Hadamard transform
static void inPlaceTransform(fixed_cell_t *tr, col_t size){ // tr must have 1 for false, -1 for true, can accept partially transformed inputs too
  if (size == 1) return;
  col_t h = size/2;
  for (col_t i = 0; i < h; i++){
    fixed_cell_t a = tr[i];
    fixed_cell_t b = tr[i+h];
    tr[i] = a + b;
    tr[i+h] = a - b;
  }
  inPlaceTransform(tr, h);
  inPlaceTransform(tr + h, h);
}

void bdd_get_transformWithoutRnd(bdd_t val, fixed_cell_t ret[NUM_NORND_COLS]){
  bdd_op_getSumRNDs(val, ret);
  inPlaceTransform(ret, NUM_NORND_COLS);
}
