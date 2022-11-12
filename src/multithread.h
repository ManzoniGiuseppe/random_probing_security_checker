//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _MULTITHREAD_H_
#define _MULTITHREAD_H_

#include <stddef.h>
#include "types.h"


//#define NUM_THREADS


// only the value itself
#define MULTITHREAD_SYNC_VAL   1
// sync the two threads that access that value
#define MULTITHREAD_SYNC_THREAD  2
// sync with all threads
#define MULTITHREAD_SYNC_ALL   3


#if NUM_THREADS > 1
  #include <threads.h>
  #include <stdatomic.h>
#endif


// mutex

#if NUM_THREADS == 1
  typedef int multithread_mtx_t;
  inline bool multithread_mtx_init(__attribute__((unused)) multithread_mtx_t *lock){ return 1; }
  T__THREAD_SAFE inline bool multithread_mtx_lock(__attribute__((unused)) multithread_mtx_t *lock){ return 1; }
  T__THREAD_SAFE inline bool multithread_mtx_unlock(__attribute__((unused)) multithread_mtx_t *lock){ return 1; }
  inline void multithread_mtx_destroy(__attribute__((unused)) multithread_mtx_t *lock){ }
#else
  typedef mtx_t multithread_mtx_t;
  inline bool multithread_mtx_init(multithread_mtx_t *lock){ return mtx_init(lock, mtx_plain) == thrd_success; }
  T__THREAD_SAFE inline bool multithread_mtx_lock(multithread_mtx_t *lock){ return mtx_lock(lock) == thrd_success; }
  T__THREAD_SAFE inline bool multithread_mtx_unlock(multithread_mtx_t *lock){ return mtx_unlock(lock) == thrd_success; }
  inline void multithread_mtx_destroy(multithread_mtx_t *lock){ mtx_destroy(lock); }
#endif


// thread

#if NUM_THREADS == 1
  inline void multithread_thr_parallel(void *info, void (*fn)(void *info)){ fn(info); }
  inline int multithread_thr_getId(void){ return -1; }
#else
  int multithread_thr_getId(void);

  __attribute__ ((noreturn)) T__THREAD_SAFE inline void multithread_thr_exit(int ret){ thrd_exit(ret); }
  typedef struct {
    void (*fn)(void *info);
    int th;
    void *info;
  } multithread_thr_parallel_t;

  int multithread_thr_parallel_i(void *p);

  inline void multithread_thr_parallel(void *info, void (*fn)(void *info)){
    if(multithread_thr_getId() != -1) FAIL("multithread_thr_parallel can only be invoked by the main thread.\n")

    thrd_t threads[NUM_THREADS];
    multithread_thr_parallel_t init[NUM_THREADS];
    for(int i = 0; i < NUM_THREADS; i++){
      init[i].fn = fn;
      init[i].th = i;
      init[i].info = info;
      if(thrd_create(threads + i, multithread_thr_parallel_i, init + i) != thrd_success) FAIL("multithread failed to create thread %d\n", i)
    }

    for(int i = 0; i < NUM_THREADS; i++){
      if(thrd_join(threads[i], NULL) != thrd_success) FAIL("multithread failed to join thread %d\n", i)
    }
  }
#endif

// atomic

#if NUM_THREADS == 1
  #define DEF_TYPE(MULTI_TYPE, UND_TYPE, NAME)   typedef UND_TYPE MULTI_TYPE;
  #define DEF_INIT(MULTI_TYPE, UND_TYPE, NAME)   inline void multithread_##NAME##_init(MULTI_TYPE *b, UND_TYPE val){ *b = val; }
  #define DEF_EXCHANGE(MULTI_TYPE, UND_TYPE, NAME)  T__THREAD_SAFE inline UND_TYPE multithread_##NAME##_exchange(MULTI_TYPE *b, UND_TYPE val, __attribute__((unused)) int sync){ \
    UND_TYPE ret = *b; \
    *b = val; \
    return ret; \
  }
  #define DEF_GET(MULTI_TYPE, UND_TYPE, NAME)  inline UND_TYPE multithread_##NAME##_get(MULTI_TYPE *i, __attribute__((unused)) int sync){ return *i; }
  #define DEF_SET(MULTI_TYPE, UND_TYPE, NAME)  inline void multithread_##NAME##_set(MULTI_TYPE *i, UND_TYPE val, __attribute__((unused)) int sync){ *i = val; }
  #define DEF_FETCH_ADD(MULTI_TYPE, UND_TYPE, NAME)  inline UND_TYPE multithread_##NAME##_fetch_add(MULTI_TYPE *i, UND_TYPE toAdd, __attribute__((unused)) int sync){ \
    UND_TYPE ret = *i; \
    *i += toAdd; \
    return ret; \
  }
#else
  #define DEF_TYPE(MULTI_TYPE, UND_TYPE, NAME)   typedef _Atomic UND_TYPE MULTI_TYPE;
  #define DEF_INIT(MULTI_TYPE, UND_TYPE, NAME)   inline void multithread_##NAME##_init(MULTI_TYPE *b, UND_TYPE val){ atomic_init(b, val); }
  #define DEF_EXCHANGE(MULTI_TYPE, UND_TYPE, NAME)  T__THREAD_SAFE inline UND_TYPE multithread_##NAME##_exchange(MULTI_TYPE *b, UND_TYPE val, int sync){ \
    if(sync == MULTITHREAD_SYNC_VAL) \
      return atomic_exchange_explicit(b, val, memory_order_relaxed); \
    else if(sync == MULTITHREAD_SYNC_THREAD) \
      return atomic_exchange_explicit(b, val, memory_order_acq_rel); \
    else /* sync == MULTITHREAD_SYNC_ALL */ \
      return atomic_exchange_explicit(b, val, memory_order_seq_cst); \
  }

  #define DEF_GET(MULTI_TYPE, UND_TYPE, NAME)  inline UND_TYPE multithread_##NAME##_get(MULTI_TYPE *i, int sync){ \
    if(sync == MULTITHREAD_SYNC_VAL) \
      return atomic_load_explicit(i, memory_order_relaxed); \
    else if(sync == MULTITHREAD_SYNC_THREAD) \
      return atomic_load_explicit(i, memory_order_acquire); \
    else /* sync == MULTITHREAD_SYNC_ALL */ \
      return atomic_load_explicit(i, memory_order_seq_cst); \
  }
  #define DEF_SET(MULTI_TYPE, UND_TYPE, NAME)  inline void multithread_##NAME##_set(MULTI_TYPE *i, UND_TYPE val, __attribute__((unused)) int sync){ \
    if(sync == MULTITHREAD_SYNC_VAL) \
      return atomic_store_explicit(i, val, memory_order_relaxed); \
    else if(sync == MULTITHREAD_SYNC_THREAD) \
      return atomic_store_explicit(i, val, memory_order_release); \
    else /* sync == MULTITHREAD_SYNC_ALL */ \
      return atomic_store_explicit(i, val, memory_order_seq_cst); \
  }
  #define DEF_FETCH_ADD(MULTI_TYPE, UND_TYPE, NAME)  inline UND_TYPE multithread_##NAME##_fetch_add(MULTI_TYPE *i, UND_TYPE toAdd, int sync){ \
    if(sync == MULTITHREAD_SYNC_VAL) \
      return atomic_fetch_add_explicit(i, toAdd, memory_order_relaxed); \
    else if(sync == MULTITHREAD_SYNC_THREAD) \
      return atomic_fetch_add_explicit(i, toAdd, memory_order_acq_rel); \
    else /* sync == MULTITHREAD_SYNC_ALL */ \
      return atomic_fetch_add_explicit(i, toAdd, memory_order_seq_cst); \
  }
#endif

#define DEF_COMMON(MULTI_TYPE, UND_TYPE, NAME)  \
  DEF_TYPE(MULTI_TYPE, UND_TYPE, NAME)  \
  DEF_INIT(MULTI_TYPE, UND_TYPE, NAME)  \
  DEF_EXCHANGE(MULTI_TYPE, UND_TYPE, NAME) \
  DEF_GET(MULTI_TYPE, UND_TYPE, NAME) \
  DEF_SET(MULTI_TYPE, UND_TYPE, NAME)

#define DEF_ARITH(MULTI_TYPE, UND_TYPE, NAME)  \
  DEF_FETCH_ADD(MULTI_TYPE, UND_TYPE, NAME)

DEF_COMMON(multithread_bool_t, bool, bool)
DEF_COMMON(multithread_fu64_t, uint_fast64_t, fu64)
DEF_COMMON(multithread_lu32_t, uint_least32_t, lu32)
DEF_COMMON(multithread_double_t, double, double)

DEF_ARITH(multithread_fu64_t, uint_fast64_t, fu64)

#undef DEF_INIT
#undef DEF_EXCHANGE
#undef DEF_GET
#undef DEF_SET
#undef DEF_FETCH_ADD

#undef DEF_COMMON
#undef DEF_ARITH

#endif // _MULTITHREAD_H_
