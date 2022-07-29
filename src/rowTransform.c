#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "mem.h"
#include "row.h"
#include "rowTransform.h"
#include "hashSet.h"




#if D * NUM_OUTS > 64
  #error "Current algprithm doesn't support D * NUM_OUTS > 64"
#endif
#if D * NUM_INS > 64
  #error "Current typing doesn't support D * NUM_INS > 64"
#endif


// -- reserve transform


typedef struct {
  hash_l_t hash; // mandatory here.
  fixed_cell_t transform[NUM_NORND_COLS];
} reservation_tran_t;

static hashSet_t htTran;

#define HTRAN_INITIAL_SIZE_BITS 10

static void __attribute__ ((constructor)) allocReservationTran(){
  htTran = hashSet_new(HTRAN_INITIAL_SIZE_BITS, sizeof(reservation_tran_t), "htTran for rowTransform");
}
static void __attribute__ ((destructor)) freeReservationTrans(){
  hashSet_delete(htTran);
}


// -- reserve row


#define NUM_DIRECT_SUB_ROWS  (MAX_COEFF+1 + NUM_OUTS*D)

typedef struct {
  hash_l_t hash; // mandatory here.
  hash_s_t transform_hash;
  hash_s_t direct_sub_row_hash[NUM_DIRECT_SUB_ROWS];
} reservation_row_t;


//-- assoc


typedef struct {
  row_t index;
  hash_s_t transform_hash;
  hash_s_t row_hash;
} assoc_t;

static size_t assoc_items;
static assoc_t *assoc;
static hash_s_t row_hash_size;
static bool finalized;

static void __attribute__ ((constructor)) allocAssoc(){
  assoc = mem_calloc(sizeof(assoc_t), 1ll << ROWTRANSFORM_ASSOC_BITS, "assoc for rowTransform");
  for(uint64_t i = 0; i < 1ll << ROWTRANSFORM_ASSOC_BITS; i++){
    assoc[i].transform_hash = ~(hash_s_t)0;
    assoc[i].row_hash = ~(hash_s_t)0;
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

  hash_s_t r = 0;
  for(int i = 0; i < 64 ; i+=ROWTRANSFORM_ASSOC_BITS)
    r ^= (hash >> i) & ((1ll << ROWTRANSFORM_ASSOC_BITS) -1);
  return r;
}

static uint64_t assoc_hash_requests;
static uint64_t assoc_hash_lookups;


//-- rowTransform_insert

static hash_s_t rowTransform_find(row_t index){
  assoc_hash_requests++;
  hash_s_t counter = 0;

  while(1){
if(counter == 10) printf("DBG: rowTransform: find: counter to 10\n");
if(counter == 100) printf("DBG: rowTransform: find: counter to 100\n");

    hash_s_t hash = assoc_get_hash(index, counter++);
    for(int i = 0; i < ASSOC_HT_BLOCK; i++){
      hash_s_t hash_it = (hash &~(ASSOC_HT_BLOCK-1)) | ((hash+i) & (ASSOC_HT_BLOCK-1));
      assoc_t *it = &assoc[ hash_it ];
      assoc_hash_lookups++;
      if(it->transform_hash == ~(hash_s_t)0 || row_eq(it->index, index))
        return hash_it;
    }
  }
}

static void htTran_increaseSize(void){
  hashSet_t htTran_new = hashSet_new(htTran->size + 1, sizeof(reservation_tran_t), "htTran for rowTransform");
  hash_s_t *newPos = mem_calloc(sizeof(hash_s_t), 1ll << htTran->size, "htTran translator for rowTransform");

  for(hash_s_t i = 0; i < 1ll<<htTran->size; i++)
    if(hashSet_validPos(htTran, i))
      if(!hashSet_tryAdd(htTran_new, hashSet_getKey(htTran, i), newPos + i))
        FAIL("rowTransform: couldn't add a row to the htTran_new! fill=%f, conflictRate=%f\n", hashSet_dbg_fill(htTran_new), hashSet_dbg_hashConflictRate(htTran_new));

  hashSet_delete(htTran);
  htTran = htTran_new;

  for(hash_s_t i = 0; i < 1ll<<ROWTRANSFORM_ASSOC_BITS; i++){
    assoc_t *it = & assoc[i];
    if(it->transform_hash != ~(hash_s_t)0)
      it->transform_hash = newPos[it->transform_hash];
  }

  mem_free(newPos);
}

void rowTransform_insert(row_t index, fixed_cell_t transform[NUM_NORND_COLS]){
  if(finalized) FAIL("rowTransform: Asking to insert a row after finalization!\n")

  assoc_t *it = & assoc[rowTransform_find(index)];
  if(it->transform_hash != ~(hash_s_t)0)
    FAIL("rowTransform: Asking to insert a row that is already present!\n")
  assoc_items++;
  it->index = index;

  reservation_tran_t key;
  memset(&key, 0, sizeof(reservation_tran_t));
  memcpy(key.transform, transform, sizeof(fixed_cell_t) * NUM_NORND_COLS);

  if(!hashSet_tryAdd(htTran, &key, &it->transform_hash)){
    htTran_increaseSize();
    printf("rowTransform: htTran resized.\n");
    if(!hashSet_tryAdd(htTran, &key, &it->transform_hash)){
      FAIL("rowTransform: couldn't add a row to htTran even after doubling the size! fill=%f, conflictRate=%f\n", hashSet_dbg_fill(htTran), hashSet_dbg_hashConflictRate(htTran));
    }
  }
}

//-- rowTransform_finalizze

static hash_s_t rowTransform_assoc_hash(row_t index){
  hash_s_t it = rowTransform_find(index);
  if(assoc[it].transform_hash == ~(hash_s_t)0)
    FAIL("rowTransform: Asking to get a missing row!\n")
  return it;
}

static bool rowTransform_finalizze__tryWithSet(void){
  hashSet_t htRow = hashSet_new(ROWTRANSFORM_ASSOC_BITS, sizeof(reservation_row_t), "htRow for rowTransform");

  row_t row = row_first();
  do{
    assoc_t *it = & assoc[rowTransform_assoc_hash(row)];

    reservation_row_t key;
    memset(&key, 0, sizeof(reservation_row_t));
    key.transform_hash = it->transform_hash;
    int i = 0;
    ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
      key.direct_sub_row_hash[i++] = assoc[rowTransform_assoc_hash(sub)].row_hash;
    })

    if(!hashSet_tryAdd(htRow, &key, &it->row_hash)){ // fail
      hashSet_delete(htRow);
      return 0;
    }
  }while(row_tryNextProbeAndOut(& row));

  row_hash_size = htRow->items;

  hashSet_delete(htRow);
  return 1;
}

static void rowTransform_finalizze__compact(void){
  hash_s_t count = 0;

  hash_s_t *pos_plus1 = mem_calloc(sizeof(hash_s_t), 1ll<<ROWTRANSFORM_ASSOC_BITS, "htRow for rowTransform");
  for(hash_s_t i = 0; i < 1ll<<ROWTRANSFORM_ASSOC_BITS; i++){
    assoc_t *it = & assoc[i];
    if(it->row_hash != ~(hash_s_t)0){
      if(pos_plus1[it->row_hash] == 0){
        pos_plus1[it->row_hash] = ++count;
      }
      it->row_hash = pos_plus1[it->row_hash]-1;
    }
  }

  if(count != row_hash_size) FAIL("rowTransform: size of the hash is wrong! count=%d, size=%d\n", count, row_hash_size)
  mem_free(pos_plus1);
}


void rowTransform_finalizze(void){
  if(finalized) FAIL("rowTransform: Asking to finalize after finalization!\n")

  if(rowTransform_finalizze__tryWithSet()){
    rowTransform_finalizze__compact();
    finalized = 1;
    return;
  }

  for(hash_s_t i = 0; i < 1ll<<ROWTRANSFORM_ASSOC_BITS; i++)
    assoc[i].row_hash = i;
  row_hash_size = 1ll<<ROWTRANSFORM_ASSOC_BITS;
  finalized = 1;
}


//-- rowTransform_transform_hash

hash_s_t rowTransform_transform_hash(row_t index){
  if(!finalized) FAIL("rowTransform: Asking to obtain an hash before finalization!\n")
  return assoc[rowTransform_assoc_hash(index)].transform_hash;
}

//-- rowTransform_transform_hash_size

hash_s_t rowTransform_transform_hash_size(void){
  if(!finalized) FAIL("rowTransform: Asking to obtain the size of transform's hashes before finalization!\n")
  return 1ll << htTran->size;
}

//-- rowTransform_row_hash_size

hash_s_t rowTransform_row_hash_size(void){
  if(!finalized) FAIL("rowTransform: Asking to obtain the size of row's hashes before finalization!\n")
  return row_hash_size;
}

//-- rowTransform_row_hash

hash_s_t rowTransform_row_hash(row_t index){
  if(!finalized) FAIL("rowTransform: Asking to obtain an hash before finalization!\n")
  return assoc[rowTransform_assoc_hash(index)].row_hash;
}

//-- rowTransform_get

void rowTransform_get(row_t row, fixed_cell_t ret_transform[NUM_NORND_COLS]){
  if(!finalized) FAIL("rowTransform: Asking to obtain the transform before finalization!\n")
  reservation_tran_t *elem = hashSet_getKey(htTran, rowTransform_transform_hash(row));

  for(int i = 0; i < NUM_NORND_COLS; i++){
    ret_transform[i] = elem->transform[i];
  }
}




double rowTransform_transform_dbg_fill(void){ return hashSet_dbg_fill(htTran); }
double rowTransform_transform_dbg_hashConflictRate(void){ return hashSet_dbg_hashConflictRate(htTran); }

double rowTransform_row_dbg_allocRate(void){
  return ((double) row_hash_size) / (1ll << ROWTRANSFORM_ASSOC_BITS);
}
double rowTransform_assoc_dbg_fill(void){
  return ((double) assoc_items) / (1ll << ROWTRANSFORM_ASSOC_BITS);
}
double rowTransform_assoc_dbg_hashConflictRate(void){
  return ((double) assoc_hash_lookups - assoc_hash_requests) / assoc_hash_requests;
}

