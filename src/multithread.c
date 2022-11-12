//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "multithread.h"


//#define NUM_THREADS


#if NUM_THREADS > 1
  static thread_local int thread = -1;
  int multithread_thr_getId(void){
    return thread;
  }

  int multithread_thr_parallel_i(void *p){
    multithread_thr_parallel_t *v = (multithread_thr_parallel_t*)p;
    thread = v->th;
    v->fn(v->info);
    return 0;
  }
#endif
