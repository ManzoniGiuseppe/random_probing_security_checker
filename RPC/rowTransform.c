#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "mem.h"
#include "rowTransform.h"




#if D * NUM_OUTS > 64
  #error "Current algprithm doesn't support D * NUM_OUTS > 64"
#endif
#if D * NUM_INS > 64
  #error "Current typing doesn't support D * NUM_INS > 64"
#endif



typedef uint64_t hash_t;


// -- storage


typedef struct {
  hash_t base_hash; // 0 for unused
  fixed_cell_t transform[NUM_NORND_COLS];
} reservation_t;

static reservation_t *reservation;
static size_t reservation_items;

static void __attribute__ ((constructor)) allocReservation(){
  reservation = mem_calloc(sizeof(reservation_t), 1ll << ROWTRANSFORM_RESERVATION_BITS, "reservation for rowTransform");
}
static void __attribute__ ((destructor)) freeReservation(){
  mem_free(reservation);
}

#define ROT(x, pos) (((x) << (pos)) | ((x) >> (-(pos) & 63)))
// must be pow2
#define RESERVATION_HT_BLOCK 32

#if RESERVATION_HT_BLOCK > 1ll << ROWTRANSFORM_RESERVATION_BITS
  #undef RESERVATION_HT_BLOCK
  #define RESERVATION_HT_BLOCK (1ll << ROWTRANSFORM_RESERVATION_BITS)
#endif

static inline hash_t reservation_get_hash(fixed_cell_t transform[NUM_NORND_COLS], hash_t counter, hash_t* base_hash){
  hash_t hash = 0;

  for(int i = 0; i < NUM_NORND_COLS; i++){
    hash_t it = transform[i] + counter;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
  }
  hash = (hash * 0xB0000B) ^ ROT(hash, 17);

  *base_hash = hash != 0 ? hash : 1; // ensure base_hash is 0 only when uninited
  return (hash >> (64-ROWTRANSFORM_RESERVATION_BITS)) ^ (hash & ((1ll << ROWTRANSFORM_RESERVATION_BITS)-1));
}

static uint64_t reservation_hash_requests;
static uint64_t reservation_hash_lookups;

static hash_t reservation_get_transform_hash(fixed_cell_t transform[NUM_NORND_COLS]){
  reservation_hash_requests++;
  hash_t counter = 0;
  while(1){
    hash_t base_hash;
    hash_t hash = reservation_get_hash(transform, counter++, & base_hash);

    for(int i = 0; i < RESERVATION_HT_BLOCK; i++){
      hash_t hash_it = (hash &~(RESERVATION_HT_BLOCK-1)) | ((hash+i) & (RESERVATION_HT_BLOCK-1));
      reservation_t* it = &reservation[ hash_it ];
      reservation_hash_lookups++;
      if(it->base_hash == 0){
        reservation_items++;
        for(int i = 0; i < NUM_NORND_COLS; i++){
          it->transform[i] = transform[i];
        }
        it->base_hash = base_hash;
        return hash_it;
      }

      if(it->base_hash != base_hash) continue; // speed up
      for(int i = 0; i < NUM_NORND_COLS; i++)
        if(it->transform[i] != transform[i])
          continue;

      // implicit  if(it->transform == transform)
      return hash_it;
    }
  }
}


//-- assoc


typedef struct {
  row_t index;
  uint64_t transform_hash;
} assoc_t;

static size_t assoc_items;
static assoc_t *assoc;

static void __attribute__ ((constructor)) allocAssoc(){
  assoc = mem_calloc(sizeof(assoc_t), 1ll << ROWTRANSFORM_ASSOC_BITS, "assoc for rowTransform");
  for(uint64_t i = 0; i < 1ll << ROWTRANSFORM_ASSOC_BITS; i++)
    assoc[i].transform_hash = (uint64_t) -1;
}
static void __attribute__ ((destructor)) freeAssoc(){
  mem_free(assoc);
}

// must be pow2
#define ASSOC_HT_BLOCK 32

#if ASSOC_HT_BLOCK > 1ll << ROWTRANSFORM_ASSOC_BITS
  #undef ASSOC_HT_BLOCK
  #define ASSOC_HT_BLOCK (1ll << ROWTRANSFORM_ASSOC_BITS)
#endif

static inline hash_t assoc_get_hash(row_t index, hash_t counter){
  hash_t hash = 0;

  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    hash_t it = index.values[i] + counter;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  }

  return (hash >> (64-ROWTRANSFORM_ASSOC_BITS)) ^ (hash & ((1ll << ROWTRANSFORM_ASSOC_BITS)-1));
}

static uint64_t assoc_hash_requests;
static uint64_t assoc_hash_lookups;


//-- rowTransform_insert


void rowTransform_insert(row_t index, fixed_cell_t transform[NUM_NORND_COLS]){
  assoc_hash_requests++;
  hash_t counter = 0;
  while(1){
    hash_t hash = assoc_get_hash(index, counter++);
    for(int i = 0; i < ASSOC_HT_BLOCK; i++){
      hash_t hash_it = (hash &~(ASSOC_HT_BLOCK-1)) | ((hash+i) & (ASSOC_HT_BLOCK-1));
      assoc_t *it = &assoc[ hash_it ];
      assoc_hash_lookups++;
      if(it->transform_hash == (uint64_t) -1){
        assoc_items++;
        it->index = index;
        it->transform_hash = reservation_get_transform_hash(transform);
        return;
      }

      if(row_eq(it->index, index)) FAIL("rowTransform: Asking to insert a row that is already present!")
    }
  }
}


//-- rowTransform_transform_hash


hash_t rowTransform_row_hash(row_t index){
  assoc_hash_requests++;
  hash_t counter = 0;
  while(1){
    hash_t hash  = assoc_get_hash(index, counter++);
    for(int i = 0; i < ASSOC_HT_BLOCK; i++){
      hash_t hash_it = (hash &~(ASSOC_HT_BLOCK-1)) | ((hash+i) & (ASSOC_HT_BLOCK-1));
      assoc_t *it = & assoc[ hash_it ];
      assoc_hash_lookups++;
      if(it->transform_hash == (uint64_t) -1) FAIL("rowTransform: missing!") // TODO contidtion

      if(row_eq(it->index, index))
        return hash_it;
    }
  }
}

hash_t rowTransform_transform_hash(row_t index){
  return assoc[rowTransform_row_hash(index)].transform_hash;
}


//-- rowTransform_get


void rowTransform_get(row_t row, fixed_cell_t ret_transform[NUM_NORND_COLS]){
  reservation_t *elem = & reservation[rowTransform_transform_hash(row)];

  for(int i = 0; i < NUM_NORND_COLS; i++){
    ret_transform[i] = elem->transform[i];
  }
}






double rowTransform_transform_dbg_fill(void){
  return ((double) reservation_items) / (1ll << ROWTRANSFORM_RESERVATION_BITS);
}
double rowTransform_transform_dbg_hashConflictRate(void){
  return ((double) reservation_hash_lookups - reservation_hash_requests) / reservation_hash_requests;
}
double rowTransform_row_dbg_fill(void){
  return ((double) assoc_items) / (1ll << ROWTRANSFORM_ASSOC_BITS);
}
double rowTransform_row_dbg_hashConflictRate(void){
  return ((double) assoc_hash_lookups - assoc_hash_requests) / assoc_hash_requests;
}

