#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "mem.h"
#include "row.h"
#include "rowTransform.h"




#if D * NUM_OUTS > 64
  #error "Current algprithm doesn't support D * NUM_OUTS > 64"
#endif
#if D * NUM_INS > 64
  #error "Current typing doesn't support D * NUM_INS > 64"
#endif


// -- reserve transform


typedef struct {
  hash_l_t base_hash; // 0 for unused
  fixed_cell_t transform[NUM_NORND_COLS];
} reservation_trans_t;

static reservation_trans_t *reservation_trans;
static size_t reservation_trans_items;

static void __attribute__ ((constructor)) allocReservationTrans(){
  reservation_trans = mem_calloc(sizeof(reservation_trans_t), 1ll << ROWTRANSFORM_TRANSFORM_BITS, "reservation_trans for rowTransform");
}
static void __attribute__ ((destructor)) freeReservationTrans(){
  mem_free(reservation_trans);
}

#define ROT(x, pos) (((x) << (pos)) | ((x) >> (-(pos) & 63)))
// must be pow2
#define RESERVATION_TRANS_HT_BLOCK 32

#if RESERVATION_TRANS_HT_BLOCK > 1ll << ROWTRANSFORM_TRANSFORM_BITS
  #undef RESERVATION_TRANS_HT_BLOCK
  #define RESERVATION_TRANS_HT_BLOCK (1ll << ROWTRANSFORM_TRANSFORM_BITS)
#endif

static inline hash_s_t reservation_trans_get_hash(fixed_cell_t transform[NUM_NORND_COLS], hash_s_t counter, hash_l_t* base_hash){
  hash_l_t hash = 0;

  for(int i = 0; i < NUM_NORND_COLS; i++){
    hash_l_t it = transform[i] + counter;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
  }
  hash = (hash * 0xB0000B) ^ ROT(hash, 17);

  *base_hash = hash != 0 ? hash : 1; // ensure base_hash is 0 only when uninited
  return (hash >> (64-ROWTRANSFORM_TRANSFORM_BITS)) ^ (hash & ((1ll << ROWTRANSFORM_TRANSFORM_BITS)-1));
}

static uint64_t reservation_trans_hash_requests;
static uint64_t reservation_trans_hash_lookups;

static hash_s_t reservation_trans_get_transform_hash(fixed_cell_t transform[NUM_NORND_COLS]){
  reservation_trans_hash_requests++;
  hash_s_t counter = 0;
  while(1){
    hash_l_t base_hash;
    hash_s_t hash = reservation_trans_get_hash(transform, counter++, & base_hash);

    for(int i = 0; i < RESERVATION_TRANS_HT_BLOCK; i++){
      hash_s_t hash_it = (hash &~(RESERVATION_TRANS_HT_BLOCK-1)) | ((hash+i) & (RESERVATION_TRANS_HT_BLOCK-1));
      reservation_trans_t* it = &reservation_trans[ hash_it ];
      reservation_trans_hash_lookups++;
      if(it->base_hash == 0){
        reservation_trans_items++;
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


// -- reserve row


#define NUM_DIRECT_SUB_ROWS  (MAX_COEFF+1 + NUM_OUTS*D)

typedef struct {
  hash_l_t base_hash; // 0 for unused
  hash_s_t transform_hash;
  hash_s_t direct_sub_row_hash[NUM_DIRECT_SUB_ROWS];
} reservation_row_t;

static reservation_row_t *reservation_row;
static size_t reservation_row_items;

static void __attribute__ ((constructor)) allocReservationRow(){
  reservation_row = mem_calloc(sizeof(reservation_row_t), 1ll << ROWTRANSFORM_ROW_BITS, "reservation_row for rowTransform");
}
static void __attribute__ ((destructor)) freeReservationRow(){
  mem_free(reservation_row);
}

#define ROT(x, pos) (((x) << (pos)) | ((x) >> (-(pos) & 63)))
// must be pow2
#define RESERVATION_ROW_HT_BLOCK 32

#if RESERVATION_ROW_HT_BLOCK > 1ll << ROWTRANSFORM_ROW_BITS
  #undef RESERVATION_ROW_HT_BLOCK
  #define RESERVATION_ROW_HT_BLOCK (1ll << ROWTRANSFORM_ROW_BITS)
#endif

static inline hash_s_t reservation_row_get_hash(row_t row, hash_s_t row_transform_hash, hash_s_t direct_sub_row_hash[NUM_DIRECT_SUB_ROWS], hash_s_t counter, hash_l_t* base_hash){
  hash_l_t hash = row_transform_hash + counter;

  for(int i = 0; i < NUM_DIRECT_SUB_ROWS; i++){
    hash ^= direct_sub_row_hash[i];
    hash += ROT(hash, ROWTRANSFORM_ROW_BITS);
  }
  hash = (hash * 0xB0000B) ^ ROT(hash, 17);

  *base_hash = hash != 0 ? hash : 1; // ensure base_hash is 0 only when uninited
  return (hash >> (64-ROWTRANSFORM_ROW_BITS)) ^ (hash & ((1ll << ROWTRANSFORM_ROW_BITS)-1));
}

static uint64_t reservation_row_hash_requests;
static uint64_t reservation_row_hash_lookups;

static hash_s_t reservation_row_get_transform_hash(row_t row, hash_s_t row_transform_hash, hash_s_t direct_sub_row_hash[NUM_DIRECT_SUB_ROWS]){
  reservation_row_hash_requests++;
  hash_s_t counter = 0;
  while(1){
    hash_l_t base_hash;
    hash_s_t hash = reservation_row_get_hash(row, row_transform_hash, direct_sub_row_hash, counter++, & base_hash);

    for(int i = 0; i < RESERVATION_ROW_HT_BLOCK; i++){
      hash_s_t hash_it = (hash &~(RESERVATION_ROW_HT_BLOCK-1)) | ((hash+i) & (RESERVATION_ROW_HT_BLOCK-1));
      reservation_row_t* it = &reservation_row[ hash_it ];
      reservation_row_hash_lookups++;
      if(it->base_hash == 0){
        reservation_row_items++;
        for(int i = 0; i < NUM_DIRECT_SUB_ROWS; i++){
          it->direct_sub_row_hash[i] = direct_sub_row_hash[i];
        }
        it->transform_hash = row_transform_hash;
        it->base_hash = base_hash;
        return hash_it;
      }

      if(it->base_hash != base_hash || it->transform_hash != row_transform_hash) continue; // speed up
      for(int i = 0; i < NUM_DIRECT_SUB_ROWS; i++)
        if(it->direct_sub_row_hash[i] != direct_sub_row_hash[i])
          continue;

      // implicit  if(it->transform == transform)
      return hash_it;
    }
  }
}


//-- assoc


typedef struct {
  row_t index;
  hash_s_t transform_hash;
  hash_s_t row_hash;
} assoc_t;

static size_t assoc_items;
static assoc_t *assoc;

static void __attribute__ ((constructor)) allocAssoc(){
  assoc = mem_calloc(sizeof(assoc_t), 1ll << ROWTRANSFORM_ASSOC_BITS, "assoc for rowTransform");
  for(uint64_t i = 0; i < 1ll << ROWTRANSFORM_ASSOC_BITS; i++){
    assoc[i].transform_hash = (hash_s_t) -1;
    assoc[i].row_hash = (hash_s_t) -1;
  }
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

static inline hash_s_t assoc_get_hash(row_t index, hash_s_t counter){
  hash_l_t hash = 0;

  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    hash_l_t it = index.values[i] + counter;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  }

  return (hash >> (64-ROWTRANSFORM_ASSOC_BITS)) ^ (hash & ((1ll << ROWTRANSFORM_ASSOC_BITS)-1));
}

static uint64_t assoc_hash_requests;
static uint64_t assoc_hash_lookups;


//-- rowTransform_transform_hash

static hash_s_t rowTransform_assoc_hash(row_t index){
  assoc_hash_requests++;
  hash_s_t counter = 0;
  while(1){
    hash_s_t hash  = assoc_get_hash(index, counter++);
    for(int i = 0; i < ASSOC_HT_BLOCK; i++){
      hash_s_t hash_it = (hash &~(ASSOC_HT_BLOCK-1)) | ((hash+i) & (ASSOC_HT_BLOCK-1));
      assoc_t *it = & assoc[ hash_it ];
      assoc_hash_lookups++;
      if(it->transform_hash == (uint64_t) -1) FAIL("rowTransform: missing!")

      if(row_eq(it->index, index))
        return hash_it;
    }
  }
}

hash_s_t rowTransform_transform_hash(row_t index){
  return assoc[rowTransform_assoc_hash(index)].transform_hash;
}

//-- rowTransform_row_hash

hash_s_t rowTransform_row_hash(row_t index){
  return assoc[rowTransform_assoc_hash(index)].row_hash;
}

//-- rowTransform_insert

void rowTransform_insert(row_t index, fixed_cell_t transform[NUM_NORND_COLS]){
  assoc_hash_requests++;
  hash_s_t counter = 0;
  while(1){
    hash_s_t hash = assoc_get_hash(index, counter++);
    for(int i = 0; i < ASSOC_HT_BLOCK; i++){
      hash_s_t hash_it = (hash &~(ASSOC_HT_BLOCK-1)) | ((hash+i) & (ASSOC_HT_BLOCK-1));
      assoc_t *it = &assoc[ hash_it ];
      assoc_hash_lookups++;
      if(it->transform_hash == (hash_s_t) -1){
        assoc_items++;
        it->index = index;

        hash_s_t direct_sub_row_hash[NUM_DIRECT_SUB_ROWS];
        int i = 0;
        ITERATE_OVER_DIRECT_SUB_ROWS(index, sub, {
          if(i >= NUM_DIRECT_SUB_ROWS) FAIL("rowTransform: too many sub rows!")
          direct_sub_row_hash[i++] = rowTransform_row_hash(sub);
        })
        for(; i < NUM_DIRECT_SUB_ROWS; i++)
          direct_sub_row_hash[i] = (hash_s_t) -1;

        it->transform_hash = reservation_trans_get_transform_hash(transform);
        it->row_hash = reservation_row_get_transform_hash(index, it->transform_hash, direct_sub_row_hash);
        return;
      }

      if(row_eq(it->index, index)) FAIL("rowTransform: Asking to insert a row that is already present!")
    }
  }
}


//-- rowTransform_get


void rowTransform_get(row_t row, fixed_cell_t ret_transform[NUM_NORND_COLS]){
  reservation_trans_t *elem = & reservation_trans[rowTransform_transform_hash(row)];

  for(int i = 0; i < NUM_NORND_COLS; i++){
    ret_transform[i] = elem->transform[i];
  }
}






double rowTransform_transform_dbg_fill(void){
  return ((double) reservation_trans_items) / (1ll << ROWTRANSFORM_TRANSFORM_BITS);
}
double rowTransform_transform_dbg_hashConflictRate(void){
  return ((double) reservation_trans_hash_lookups - reservation_trans_hash_requests) / reservation_trans_hash_requests;
}
double rowTransform_row_dbg_fill(void){
  return ((double) reservation_row_items) / (1ll << ROWTRANSFORM_ROW_BITS);
}
double rowTransform_row_dbg_hashConflictRate(void){
  return ((double) reservation_row_hash_lookups - reservation_row_hash_requests) / reservation_row_hash_requests;
}
double rowTransform_assoc_dbg_fill(void){
  return ((double) assoc_items) / (1ll << ROWTRANSFORM_ASSOC_BITS);
}
double rowTransform_assoc_dbg_hashConflictRate(void){
  return ((double) assoc_hash_lookups - assoc_hash_requests) / assoc_hash_requests;
}

