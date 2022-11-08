//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

#include "types.h"



static mtx_t mutex;
static atomic_bool isExiting;


static void __attribute__ ((constructor)) init(void){
  atomic_init(&isExiting, 0);
  if(mtx_init(&mutex, mtx_plain) != thrd_success) FAIL("Error in initing the print's mutex.\n");
}
static void __attribute__ ((destructor)) deinit(void){
  mtx_destroy(&mutex);
}


void types_print_err(const char* format, ...){
  if(mtx_lock(&mutex) != thrd_success) abort();

  va_list args;
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);

  if(mtx_unlock(&mutex) != thrd_success) abort();
}

void types_print_out(const char* format, ...){
  if(mtx_lock(&mutex) != thrd_success) abort();

  va_list args;
  va_start (args, format);
  vfprintf (stdout, format, args);
  va_end (args);

  if(mtx_unlock(&mutex) != thrd_success) abort();
}

void types_exit(void){
  if(atomic_exchange(&isExiting, 1)){
    thrd_exit(1);
  }
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
