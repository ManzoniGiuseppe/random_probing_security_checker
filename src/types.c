//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "types.h"
#include "multithread.h"



//#define NUM_THREADS



static multithread_mtx_t mutex;
static multithread_bool_t isExiting;


static void __attribute__ ((constructor)) init(void){
  multithread_bool_init(&isExiting, 0);
  if(!multithread_mtx_init(&mutex)) FAIL("Error in initing the print's mutex.\n");
}
static void __attribute__ ((destructor)) deinit(void){
  multithread_mtx_destroy(&mutex);
}


void types_print_err(const char* format, ...){
  if(!multithread_mtx_lock(&mutex)) abort();

  va_list args;
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);

  if(!multithread_mtx_unlock(&mutex)) abort();
}

void types_print_out(const char* format, ...){
  if(!multithread_mtx_lock(&mutex)) abort();

  va_list args;
  va_start (args, format);
  vfprintf (stdout, format, args);
  va_end (args);

  if(!multithread_mtx_unlock(&mutex)) abort();
}

__attribute__ ((noreturn)) void types_exit(void){
  #if NUM_THREADS > 0
    if(multithread_bool_exchange(&isExiting, 1, MULTITHREAD_SYNC_THREAD)){
      multithread_thr_exit(1);
    }
  #endif
  exit(1);
}

double binomial(int n, int k){
  if(k > n-k) return binomial(n, n-k);

  double ret = 1.0;
  for(int i = n-k+1; i <= n; i++)
    ret *= i;
  for(int i = 2; i <= k; i++)
    ret /= i;

  return ret;
}
