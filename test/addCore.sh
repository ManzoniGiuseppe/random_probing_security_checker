#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

trap 'rm -rf "$dir"' EXIT
dir=$(mktemp -d) || exit 1

genExpected(){
  echo "0 0 0 0 0 0 0 0 "
  echo "1 1 1 1 1 1 1 1 "
  echo "1 1 1 1 0 0 0 0 "
  echo "0 0 1 1 0 0 1 1 "
  echo "0 0 0 0 0 0 1 1 "
}

genActual(){
cat > $dir/main.c << EOF
#include <stdio.h>
#include "addCore.h"

addCore_t c;

void print(addCore_arr_t p){
  bool truth[8];

  addCore_flattenR(c, p, 3, NULL, truth);
  for(int i = 0; i < 8; i++){
    printf("%d ", truth[i]);
  }
  printf("\n");
}

int main(){
  c = addCore_new();
  hash_t ignore = (hash_t){0};


  addCore_arr_t l0 = addCore_leaf(ignore, 0);
  print(l0);

  addCore_arr_t l1 = addCore_leaf(ignore, 1);
  print(l1);

  addCore_arr_t fn10_0 = addCore_node(c, 0, l1, l0);
  print(fn10_0);

  addCore_arr_t fn01_1 = addCore_node(c, 1, l0, l1);
  print(fn01_1);

  addCore_arr_t fn = addCore_node(c, 0, l0, fn01_1);
  print(fn);

  addCore_delete(c);
  return 0;
}
EOF

preprocFlags=""
preprocFlags="${preprocFlags} -DNUM_THREADS=0"
preprocFlags="${preprocFlags} -DMAX_NUM_TOT_INS=65"
preprocFlags="${preprocFlags} -DMEM_NUM_ALLOCS=10000"
preprocFlags="${preprocFlags} -DHASHMAP_INITIAL_BITS=7"
preprocFlags="${preprocFlags} -DHASHMAP_CONTIGUOUS_BITS=3"
preprocFlags="${preprocFlags} -DHASHMAP_HASH_ATTEMPTS=5"
preprocFlags="${preprocFlags} -DHASHMAP_SAVE_HASH_RATIO=5"
preprocFlags="${preprocFlags} -DHASHCACHE_BITS=22"
preprocFlags="${preprocFlags} -DHASHCACHE_WAYS=4"
preprocFlags="${preprocFlags} -DHASHCACHE_SPLIT_BITS=1"
preprocFlags="${preprocFlags} -DFN_CMP_STEP=0.0001"
preprocFlags="${preprocFlags} -DMAX_LEN_VAR_NAMES=100"
preprocFlags="${preprocFlags} -DMAX_FILE_SIZE=1000000"


preprocFlags="${preprocFlags} -DDBG_BDD=DBG_LVL_MAX" #DETAILED"

preprocFlags="${preprocFlags} -DDBG_HASHMAP=DBG_LVL_TOFIX"
preprocFlags="${preprocFlags} -DDBG_HASHCACHE=DBG_LVL_TOFIX"
preprocFlags="${preprocFlags} -DDBG_MEM=DBG_LVL_TOFIX"
preprocFlags="${preprocFlags} -DDBG_ADDOP=DBG_LVL_TOFIX"
preprocFlags="${preprocFlags} -DDBG_ADDCORE=DBG_LVL_TOFIX"
preprocFlags="${preprocFlags} -DDBG_BITARRAY=DBG_LVL_TOFIX"

sources=""
sources="${sources} $dir/main.c"
sources="${sources} ../src/hashMap.c"
sources="${sources} ../src/bdd.c"
sources="${sources} ../src/addOp.c"
sources="${sources} ../src/addCore.c"
sources="${sources} ../src/types.c"
sources="${sources} ../src/mem.c"
sources="${sources} ../src/hash.c"
sources="${sources} ../src/hashCache.c"

  gcc -O3 -Wall -Wextra -Werror -I ../src/ $preprocFlags $sources -lpthread -lgmp -o $dir/prog
  if [ "$?" -eq 0 ] ;then
    $dir/prog
    if [ "$?" -ne 0 ] ;then
       echo Program failed!
       exit 1
    fi
  else
     echo Compilation failed!
     exit 1
  fi
}


test(){
  genExpected >$dir/expected
  genActual >$dir/actual

  diff $dir/expected $dir/actual >$dir/diff
  if [ $? -eq 0 ] ; then
    echo "Correct!"
  else
    echo "ERR: The results differ!"
    paste <(echo 'expected:' ; cat $dir/expected) <(echo 'actual:' ; cat $dir/actual)
  fi
  echo
}

test
echo "Done."
