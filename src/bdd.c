#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include "bdd.h"
#include "mem.h"


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



bdd_t bdd_op_not(__attribute__((unused)) void* storage, bdd_t val){
  return bdd_op_neg(val);
}
bdd_t bdd_val_const(__attribute__((unused)) void* storage, bool val){
  return val ? BDD_TRUE : BDD_FALSE;
}



// -- storage

typedef struct{ bdd_t v[2]; } bdd_node_t;
typedef struct{ uint64_t opType; bdd_t sub[2]; uint64_t value; } cache_elem_t;

typedef struct {
  uint64_t hash_lookups;
  uint64_t hash_requests;
  size_t storage_items;
  uint64_t cache_items;
  uint64_t cache_gos_movements;
  uint64_t cache_gos_requests;
  bdd_node_t storage[1ll << BDD_STORAGE_BITS];
  cache_elem_t cache[1ll << BDD_CACHE_BITS];
} bdd_storage_t;

void* bdd_storage_alloc(){
  return mem_calloc(sizeof(bdd_storage_t), 1, "the storage for BDDs!");
}

void bdd_storage_free(void *storage){
  mem_free(storage);
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

  hash_t r = 0;
  for(int i = 0; i < 64 ; i+=BDD_STORAGE_BITS)
    r ^= ((hash0 ^ hash1) >> i) & ((1ll << BDD_STORAGE_BITS) -1);
  return r;
}


static inline bdd_t bdd_new_node(bdd_storage_t *s, bdd_t sub0, bdd_t sub1, shift_t input){   // add GC for when nearly empty.
  if(sub0 == sub1) return sub0; // simplify
  if(bdd_is_negated(sub0)) return bdd_op_neg(bdd_new_node(s, bdd_op_neg(sub0), bdd_op_neg(sub1), input)); // ensure it's unique: have the sub0 always positive

  s->hash_requests++;
  for(hash_t counter = 0; 1; counter++){
    hash_t hash  = bdd_new_node__h(sub0, sub1, counter);
    for(int i = 0; i < HT_BLOCK; i++){ // locality: search within the block
      hash_t hash_it = (hash & ~(HT_BLOCK-1)) | ((hash+i) & (HT_BLOCK-1));
      bdd_node_t* it = & s->storage[hash_it];
      s->hash_lookups++;
      if(it->v[0] == it->v[1]){ // a real node's children can't be the same or it'd be simplified -> must be an unused location
        s->storage_items++;
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
#define bdd_get_node(s, x)  ((s)->storage[(x) & BDD_NODE__ADDR_MASK])

double bdd_dbg_storageFill(void* storage){
  bdd_storage_t *s = storage;
  return ((double) s->storage_items) / (1ll << BDD_STORAGE_BITS);
}

double bdd_dbg_hashConflictRate(void* storage){
  bdd_storage_t *s = storage;
  return ((double) s->hash_lookups - s->hash_requests) / s->hash_requests;
}


// -- bdd_dbg_print

static void bdd_dbg_print__i(bdd_storage_t* s, bdd_t val, int padding){
  if(!bdd_is_node(val)){
    for(int i = 0; i < padding; i++) printf(" ");
    printf(val == BDD_TRUE? "TRUE\n" : "FALSE\n");
    return;
  }

  for(int i = 0; i < padding; i++) printf(" ");
  printf("%s{ %lld:\n", bdd_is_negated(val) ? "!" : "", bdd_get_input(val));

  bdd_node_t a = bdd_get_node(s, val);
  bdd_dbg_print__i(s, a.v[0], padding+2);
  bdd_dbg_print__i(s, a.v[1], padding+2);

  for(int i = 0; i < padding; i++) printf(" ");
  printf("}\n");
}

void bdd_dbg_print(void* storage, bdd_t val){
  bdd_storage_t *s = storage;
  bdd_dbg_print__i(s, val, 0);
}

// -- bdd_val_single


bdd_t bdd_val_single(void* storage, shift_t inputBit){
  bdd_storage_t *s = storage;
  return bdd_new_node(s, BDD_FALSE, BDD_TRUE, inputBit);
}


// -- cache

#define CACHE_OPTYPE__UNUSED  0
#define CACHE_OPTYPE__XOR  1
#define CACHE_OPTYPE__AND  2
#define CACHE_OPTYPE__SUM  4
#define CACHE_OPTYPE__MERGE_SUM 8

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


static inline bool cache_contains__index(bdd_storage_t *s, uint64_t opType, bdd_t sub0, bdd_t sub1, int *ret_index){
  hash_t hash = cache__h(opType, sub0, sub1);
  for(int i = 0; i < BDD_CACHE_WAYS; i++){ // associative cache
    hash_t hash_it = hash | i;
    cache_elem_t* it = & s->cache[hash_it];
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


static inline bool cache_contains(bdd_storage_t *s, uint64_t opType, bdd_t sub0, bdd_t sub1){
  int ignore;
  return cache_contains__index(s, opType, sub0, sub1, &ignore);
}

static inline uint64_t cache_get_or_set(bdd_storage_t *s, uint64_t opType, bdd_t sub0, bdd_t sub1, uint64_t value_if_missing){
  s->cache_gos_requests++;

  hash_t hash = cache__h(opType, sub0, sub1);

  int i;
  cache_elem_t to_add;
  if(cache_contains__index(s, opType, sub0, sub1, & i)) //get index
    to_add = s->cache[hash | i];
  else{
    s->cache_items++;
    to_add = (cache_elem_t){ opType, {sub0, sub1},  value_if_missing};

    // if full, remove the least accessed one.
    if(i == BDD_CACHE_WAYS) i--;
  }

  // move to create a slot for the last accessed one
  s->cache_gos_movements += i;
  for(;i > 0; i--)
    s->cache[hash | i] = s->cache[hash | (i-1)];
  s->cache[hash] = to_add;

  return to_add.value;
}

double bdd_dbg_cacheTurnover(void *storage){
  bdd_storage_t *s = storage;
  return ((double) s->cache_items) / (1ll << BDD_CACHE_BITS);
}

double bdd_dbg_cacheMovementRate(void *storage){
  bdd_storage_t *s = storage;
  return ((double) s->cache_gos_movements) / s->cache_gos_requests / (BDD_CACHE_WAYS-1);
}


// -- bdd_op_and


static bdd_t bdd_op_and__uncached(bdd_storage_t *s, bdd_t val0, bdd_t val1){
  bdd_t val[2] = { val0, val1 };

  for(int i = 0; i < 2; i++){
    if(!bdd_is_node(val[i])){
      if(val[i] == BDD_FALSE) return BDD_FALSE;
      else return val[1-i]; // return the other
    }
  }

  bdd_node_t a[2] = { bdd_get_node(s, val0), bdd_get_node(s, val1) };
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
    sub[0] = bdd_op_and(s, a[0].v[0], a[1].v[0]); // sub 0, v[0]
    sub[1] = bdd_op_and(s, a[0].v[1], a[1].v[1]);
  }else{
    sub[0] = bdd_op_and(s, a[min].v[0], val[max]);  // sub 0, v[0]
    sub[1] = bdd_op_and(s, a[min].v[1], val[max]); // only open the one with the first input, leave the other unchanged.
  }

  return bdd_new_node(s, sub[0], sub[1], i[min]);
}

bdd_t bdd_op_and(void *storage, bdd_t val0, bdd_t val1){
  bdd_storage_t *s = storage;
  uint64_t value = 0;
  if(!cache_contains(s, CACHE_OPTYPE__AND, val0, val1))
    value = bdd_op_and__uncached(s, val0, val1);
  return cache_get_or_set(s, CACHE_OPTYPE__AND, val0, val1, value);
}


// -- bdd_op_xor

static bdd_t bdd_op_xor__uncached(bdd_storage_t *s, bdd_t val0, bdd_t val1){
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
  if(count_neg == 1) return bdd_op_neg(bdd_op_xor(s, val[0], val[1]));

  bdd_node_t a[2] = { bdd_get_node(s, val[0]), bdd_get_node(s, val[1]) };
  shift_t i[2] = { bdd_get_input(val[0]), bdd_get_input(val[1]) };

  int min = i[0] < i[1] ? 0 : 1;  // parameter with the minimal input, minimal -> first.
  int max = 1-min; // parameter with the other input.

  bdd_t sub[2];
  if(i[min] == i[max]){
    sub[0] = bdd_op_xor(s, a[0].v[0], a[1].v[0]); // sub 0, v[0]
    sub[1] = bdd_op_xor(s, a[0].v[1], a[1].v[1]);
  }else{
    sub[0] = bdd_op_xor(s, a[min].v[0], val[max]);  // sub 0, v[0]
    sub[1] = bdd_op_xor(s, a[min].v[1], val[max]); // only open the one with the first input, leave the other unchanged.
  }

  return bdd_new_node(s, sub[0], sub[1], i[min]);
}

bdd_t bdd_op_xor(void *storage, bdd_t val0, bdd_t val1){
  bdd_storage_t *s = storage;
  uint64_t value = 0;
  if(!cache_contains(s, CACHE_OPTYPE__XOR, val0, val1))
    value = bdd_op_xor__uncached(s, val0, val1);
  return cache_get_or_set(s, CACHE_OPTYPE__XOR, val0, val1, value);
}


// -- bdd_op_getTransformWithoutRnd


#define BDD_FROM_CONST(x)   ((x)<0 ? bdd_op_neg((bdd_t)-(x)) : (bdd_t)(x))
#define BDD_TO_CONST(x)   (bdd_is_negated(x) ? -(int64_t)bdd_op_neg(x) : (int64_t)(x))

static inline bdd_node_t bdd_get_node_pos(bdd_storage_t *s, bdd_t val){
  bdd_node_t a = bdd_get_node(s, val);

  if(bdd_is_negated(val)){
    a.v[0] = bdd_op_neg(a.v[0]);
    a.v[1] = bdd_op_neg(a.v[1]);
  }
  return a;
}

static fixed_cell_t bdd_op_getSumRNDs__sum(bdd_storage_t *s, bdd_t val){ // do the sum recursively
  if(bdd_is_negated(val)) return -bdd_op_getSumRNDs__sum(s, bdd_op_neg(val));
  if(!bdd_is_node(val)) return -(1ll << NUM_RANDOMS);

  bdd_t value = 0;
  if(!cache_contains(s, CACHE_OPTYPE__SUM, val, val)){
    bdd_node_t a = bdd_get_node(s, val);
    fixed_cell_t cell = (bdd_op_getSumRNDs__sum(s, a.v[0]) + bdd_op_getSumRNDs__sum(s, a.v[1]))/2;  // /2 as each covers half the column space
    value = BDD_FROM_CONST(cell);
  }
  bdd_t ret = cache_get_or_set(s, CACHE_OPTYPE__SUM, val, val, value);
  if(bdd_is_node(ret)) FAIL("bdd_op_getSumRNDs__sum BUG\n");
  return BDD_TO_CONST(ret);
}

static bdd_t bdd_op_getSumRNDs__i(bdd_storage_t *s, bdd_t val);

static bdd_t bdd_op_getSumRNDs__i_uncached(bdd_storage_t *s, bdd_t val){
  shift_t input = bdd_get_input(val);

  if(!bdd_is_node(val) || input >= NUM_INS*D){
    fixed_cell_t v = bdd_op_getSumRNDs__sum(s, val);
    return BDD_FROM_CONST(v);
  }

  bdd_node_t a = bdd_get_node_pos(s, val);

  bdd_t v0 = bdd_op_getSumRNDs__i(s, a.v[0]);
  bdd_t v1 = bdd_op_getSumRNDs__i(s, a.v[1]);

  return bdd_new_node(s, v0, v1, input);
}

static bdd_t bdd_op_getSumRNDs__i(bdd_storage_t *s, bdd_t val){
  bdd_t value = 0;
  if(!cache_contains(s, CACHE_OPTYPE__MERGE_SUM, val, val))
    value = bdd_op_getSumRNDs__i_uncached(s, val);
  return cache_get_or_set(s, CACHE_OPTYPE__MERGE_SUM, val, val, value);
}

static void bdd_op_getSumRNDs__plain(bdd_storage_t *s, bdd_t val, shift_t ins_missing, fixed_cell_t *ret){ // recursively travel the BDD until you get to the randoms
  if(ins_missing == 0){
    if(bdd_is_node(val)) FAIL("bdd_op_getSumRNDs__plain BUG\n");
    *ret = BDD_TO_CONST(val);
    return;
  }

  bdd_node_t a;
  if(!bdd_is_node(val)){
    a.v[0] = a.v[1] = val;
  } else if(bdd_get_input(val) != (uint64_t) NUM_INS*D-ins_missing){ // the two a.v are the same.
    a.v[0] = a.v[1] = val;
  } else {
    a = bdd_get_node_pos(s, val);
  }

  bdd_op_getSumRNDs__plain(s, a.v[0], ins_missing-1, ret);
  bdd_op_getSumRNDs__plain(s, a.v[1], ins_missing-1, ret + (1ll << (NUM_INS*D-ins_missing)));
}

// get for each input, the sum of the values, with true=1, false=-1.
static inline void bdd_op_getSumRNDs(bdd_storage_t *s, bdd_t val, fixed_cell_t ret[NUM_NORND_COLS]){
  bdd_t v = bdd_op_getSumRNDs__i(s, val);
  bdd_op_getSumRNDs__plain(s, v, NUM_INS*D, ret);
}

/*

static void bdd_op_getSumRNDs__i_v2(bdd_storage_t *s, bdd_t val, shift_t ins_missing, bool is_pos, fixed_cell_t *ret){ // recursively travel the BDD until you get to the randoms
  if(ins_missing == 0){
    if(!is_pos) val = bdd_op_neg(val);
    *ret = bdd_op_getSumRNDs__sum(s, val);
    return;
  }

  if(bdd_is_negated(val)){
    val = bdd_op_neg(val);
    is_pos = !is_pos;
  }

  bdd_node_t a;
  if(!bdd_is_node(val)) {
    a.v[0] = a.v[1] = val;
  } else if(bdd_get_input(val) != (uint64_t) NUM_INS*D-ins_missing){ // the two a.v are the same.
    a.v[0] = a.v[1] = val;
  } else {
    a = bdd_get_node(s, val);
  }

  bdd_op_getSumRNDs__i_v2(s, a.v[0], ins_missing-1, is_pos, ret);
  bdd_op_getSumRNDs__i_v2(s, a.v[1], ins_missing-1, is_pos, ret + (1ll << (NUM_INS*D-ins_missing)));
}

// get for each input, the sum of the values, with true=1, false=-1.
static inline void bdd_op_getSumRNDs_v2(bdd_storage_t *s, bdd_t val, fixed_cell_t ret[NUM_NORND_COLS]){
  bdd_op_getSumRNDs__i_v2(s, val, NUM_INS*D, 1, ret);
}

*/

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

void bdd_get_transformWithoutRnd(void *storage, bdd_t val, fixed_cell_t ret[NUM_NORND_COLS]){
  bdd_storage_t *s = storage;
  bdd_op_getSumRNDs(s, val, ret);

  inPlaceTransform(ret, NUM_NORND_COLS);
}
