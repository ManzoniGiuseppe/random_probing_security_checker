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


// -- reserve row


typedef struct {
  hash_l_t hash; // mandatory here.
  fixed_cell_t transform[NUM_NORND_COLS];
} reservation_row_t;

static hashSet_t htRow;

#define HROW_INITIAL_SIZE_BITS 10

static void __attribute__ ((constructor)) allocReservationRow(){
  htRow = hashSet_new(HROW_INITIAL_SIZE_BITS, sizeof(reservation_row_t), "htRow for rowTransform");
}
static void __attribute__ ((destructor)) freeReservationRow(){
  hashSet_delete(htRow);
}


// -- reserve subRow


#define NUM_DIRECT_SUB_ROWS  (MAX_COEFF+1 + NUM_OUTS*D)

typedef struct {
  hash_l_t hash; // mandatory here.
  hash_s_t row_hash;
  hash_s_t direct_sub_row_hash[NUM_DIRECT_SUB_ROWS];
} reservation_subRow_t;


//-- unique


typedef struct {
  row_t index;
  hash_s_t row_hash;
  hash_s_t subRow_hash;
} unique_t;

static size_t unique_items;
static unique_t *unique;
static hash_s_t subRow_hash_size;
static bool finalized;

static void __attribute__ ((constructor)) allocUnique(){
  unique = mem_calloc(sizeof(unique_t), 1ll << ROWTRANSFORM_UNIQUE_BITS, "unique for rowTransform");
  for(uint64_t i = 0; i < 1ll << ROWTRANSFORM_UNIQUE_BITS; i++){
    unique[i].row_hash = ~(hash_s_t)0;
    unique[i].subRow_hash = ~(hash_s_t)0;
  }
}
static void __attribute__ ((destructor)) freeUnique(){
  mem_free(unique);
}



// must be pow2
#define UNIQUE_HT_BLOCK 32

#if UNIQUE_HT_BLOCK > 1ll << ROWTRANSFORM_UNIQUE_BITS
  #undef UNIQUE_HT_BLOCK
  #define UNIQUE_HT_BLOCK (1ll << ROWTRANSFORM_UNIQUE_BITS)
#endif

static inline hash_s_t unique_get_hash(row_t index, hash_s_t counter){
  hash_l_t hash = 0;

  for(int i = 0; i < ROW_VALUES_SIZE; i++){
    hash_l_t it = index.values[i] + counter;
    it = (it * 0xB0000B) ^ ROT(it, 17);
    hash ^= (it * 0xB0000B) ^ ROT(it, 17);
    hash = (hash * 0xB0000B) ^ ROT(hash, 17);
  }

  hash_s_t r = 0;
  for(int i = 0; i < 64 ; i+=ROWTRANSFORM_UNIQUE_BITS)
    r ^= (hash >> i) & ((1ll << ROWTRANSFORM_UNIQUE_BITS) -1);
  return r;
}

static uint64_t unique_hash_requests;
static uint64_t unique_hash_lookups;


//-- rowTransform_insert

static hash_s_t rowTransform_find(row_t index){
  unique_hash_requests++;
  hash_s_t counter = 0;

  while(1){
if(counter == 10) printf("DBG: rowTransform: find: counter to 10\n");
if(counter == 100) printf("DBG: rowTransform: find: counter to 100\n");

    hash_s_t hash = unique_get_hash(index, counter++);
    for(int i = 0; i < UNIQUE_HT_BLOCK; i++){
      hash_s_t hash_it = (hash &~(UNIQUE_HT_BLOCK-1)) | ((hash+i) & (UNIQUE_HT_BLOCK-1));
      unique_t *it = &unique[ hash_it ];
      unique_hash_lookups++;
      if(it->row_hash == ~(hash_s_t)0 || row_eq(it->index, index))
        return hash_it;
    }
  }
}

static void htRow_increaseSize(void){
  hashSet_t htRow_new = hashSet_new(hashSet_getNumElem(htRow) + 1, sizeof(reservation_row_t), "htRow for rowTransform");
  hash_s_t *newPos = mem_calloc(sizeof(hash_s_t), 1ll << hashSet_getNumElem(htRow), "htRow translator for rowTransform");

  for(hash_s_t i = 0; i < 1ll<<hashSet_getNumElem(htRow); i++)
    if(hashSet_validPos(htRow, i))
      if(!hashSet_tryAdd(htRow_new, hashSet_getKey(htRow, i), newPos + i))
        FAIL("rowTransform: couldn't add a row to the htRow_new! fill=%f, conflictRate=%f\n", hashSet_dbg_fill(htRow_new), hashSet_dbg_hashConflictRate(htRow_new));

  hashSet_delete(htRow);
  htRow = htRow_new;

  for(hash_s_t i = 0; i < 1ll<<ROWTRANSFORM_UNIQUE_BITS; i++){
    unique_t *it = & unique[i];
    if(it->row_hash != ~(hash_s_t)0)
      it->row_hash = newPos[it->row_hash];
  }

  mem_free(newPos);
}

void rowTransform_insert(row_t index, fixed_cell_t transform[NUM_NORND_COLS]){
  if(finalized) FAIL("rowTransform: Asking to insert a row after finalization!\n")

  unique_t *it = & unique[rowTransform_find(index)];
  if(it->row_hash != ~(hash_s_t)0)
    FAIL("rowTransform: Asking to insert a row that is already present!\n")
  unique_items++;
  it->index = index;

  reservation_row_t key;
  memset(&key, 0, sizeof(reservation_row_t));
  memcpy(key.transform, transform, sizeof(fixed_cell_t) * NUM_NORND_COLS);

  if(!hashSet_tryAdd(htRow, &key, &it->row_hash)){
    htRow_increaseSize();
    printf("rowTransform: htRow resized.\n");
    if(!hashSet_tryAdd(htRow, &key, &it->row_hash)){
      FAIL("rowTransform: couldn't add a row to htRow even after doubling the size! fill=%f, conflictRate=%f\n", hashSet_dbg_fill(htRow), hashSet_dbg_hashConflictRate(htRow));
    }
  }
}

//-- rowTransform_finalizze

static hash_s_t unique_hash(row_t index){
  hash_s_t it = rowTransform_find(index);
  if(unique[it].row_hash == ~(hash_s_t)0)
    FAIL("rowTransform: Asking to get the missing row with lower bits %lX!\n", index.values[0])
  return it;
}

static bool rowTransform_finalizze__tryWithSet(void){
  hashSet_t htSubRow = hashSet_new(ROWTRANSFORM_UNIQUE_BITS, sizeof(reservation_subRow_t), "htSubRow for rowTransform");

  row_t row = row_first();
  do{
    unique_t *it = & unique[unique_hash(row)];

    reservation_subRow_t key;
    memset(&key, 0, sizeof(reservation_subRow_t));
    key.row_hash = it->row_hash;
    int i = 0;
    ITERATE_OVER_DIRECT_SUB_ROWS(row, sub, {
      key.direct_sub_row_hash[i++] = unique[unique_hash(sub)].subRow_hash;
    })

    if(!hashSet_tryAdd(htSubRow, &key, &it->subRow_hash)){ // fail
      hashSet_delete(htSubRow);
      return 0;
    }
  }while(row_tryNextProbeAndOut(& row));

  subRow_hash_size = hashSet_getNumCurrElem(htSubRow);

  hashSet_delete(htSubRow);
  return 1;
}

static void rowTransform_finalizze__compact(void){
  hash_s_t count = 0;

  hash_s_t *pos_plus1 = mem_calloc(sizeof(hash_s_t), 1ll<<ROWTRANSFORM_UNIQUE_BITS, "htSubRow for rowTransform");
  for(hash_s_t i = 0; i < 1ll<<ROWTRANSFORM_UNIQUE_BITS; i++){
    unique_t *it = & unique[i];
    if(it->subRow_hash != ~(hash_s_t)0){
      if(pos_plus1[it->subRow_hash] == 0){
        pos_plus1[it->subRow_hash] = ++count;
      }
      it->subRow_hash = pos_plus1[it->subRow_hash]-1;
    }
  }

  if(count != subRow_hash_size) FAIL("rowTransform: size of the hash is wrong! count=%d, size=%d\n", count, subRow_hash_size)
  mem_free(pos_plus1);
}


void rowTransform_finalizze(void){
  if(finalized) FAIL("rowTransform: Asking to finalize after finalization!\n")

  if(rowTransform_finalizze__tryWithSet()){
    rowTransform_finalizze__compact();
  }else{
    for(hash_s_t i = 0; i < 1ll<<ROWTRANSFORM_UNIQUE_BITS; i++)
      unique[i].subRow_hash = i;
    subRow_hash_size = 1ll<<ROWTRANSFORM_UNIQUE_BITS;
  }

  finalized = 1;
}


//-- rowTransform_row_hash

hash_s_t rowTransform_row_hash(row_t index){
  if(!finalized) FAIL("rowTransform: Asking to obtain an hash before finalization!\n")
  return unique[unique_hash(index)].row_hash;
}

//-- rowTransform_row_hash_size

hash_s_t rowTransform_row_hash_size(void){
  if(!finalized) FAIL("rowTransform: Asking to obtain the size of row's hashes before finalization!\n")
  return 1ll << hashSet_getNumElem(htRow);
}

//-- rowTransform_subRow_hash_size

hash_s_t rowTransform_subRow_hash_size(void){
  if(!finalized) FAIL("rowTransform: Asking to obtain the size of row's hashes before finalization!\n")
  return subRow_hash_size;
}

//-- rowTransform_subRow_hash

hash_s_t rowTransform_subRow_hash(row_t index){
  if(!finalized) FAIL("rowTransform: Asking to obtain an hash before finalization!\n")
  return unique[unique_hash(index)].subRow_hash;
}

//-- rowTransform_unique_hash

hash_s_t rowTransform_unique_hash(row_t index){
  if(!finalized) FAIL("rowTransform: Asking to obtain an hash before finalization!\n")
  return unique_hash(index);
}

//-- rowTransform_unique_hash_size

hash_s_t rowTransform_unique_hash_size(void){
  if(!finalized) FAIL("rowTransform: Asking to obtain the size of row's hashes before finalization!\n")
  return 1ll<<ROWTRANSFORM_UNIQUE_BITS;
}

//-- rowTransform_get

void rowTransform_get(row_t row, fixed_cell_t ret_transform[NUM_NORND_COLS]){
  if(!finalized) FAIL("rowTransform: Asking to obtain the transform before finalization!\n")
  reservation_row_t *elem = hashSet_getKey(htRow, rowTransform_row_hash(row));

  for(int i = 0; i < NUM_NORND_COLS; i++){
    ret_transform[i] = elem->transform[i];
  }
}




double rowTransform_row_dbg_fill(void){ return hashSet_dbg_fill(htRow); }
double rowTransform_row_dbg_fillOfTot(void){ return hashSet_dbg_fill(htRow) * (1ll << hashSet_getNumElem(htRow)) / (1ll << ROWTRANSFORM_UNIQUE_BITS); }
double rowTransform_row_dbg_hashConflictRate(void){ return hashSet_dbg_hashConflictRate(htRow); }

double rowTransform_subRow_dbg_allocRate(void){
  return ((double) subRow_hash_size) / (1ll << ROWTRANSFORM_UNIQUE_BITS);
}
double rowTransform_unique_dbg_fill(void){
  return ((double) unique_items) / (1ll << ROWTRANSFORM_UNIQUE_BITS);
}
double rowTransform_unique_dbg_hashConflictRate(void){
  return ((double) unique_hash_lookups - unique_hash_requests) / unique_hash_requests;
}

