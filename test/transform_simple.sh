#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

trap 'rm -rf "$dir"' EXIT
dir=$(mktemp -d) || exit 1

genExpected(){
  local truth=$1
  local comb="$(echo $truth | sed 's/1/-1/g;s/0/1/g')"

cat > $dir/printOutputTransforms.py << EOF
#!/usr/bin/env python3

from sympy import fwht

comb=[$comb]

walsh=fwht(comb)

#print
for b in walsh:
  print(b, end=" ")
print("")

EOF

  chmod +x $dir/printOutputTransforms.py
  $dir/printOutputTransforms.py
}

genActual(){
  local truth=$1

cat > $dir/main.c << EOF
#include "parserSage.h"
#include "rowHashed.h"
#include "transformGenerator.h"


int truth[] = {$truth};

int main(){
  int size = sizeof(truth)/sizeof(int);
  int bits = 0;
  while((1 << bits) != size) bits++;

  bdd_t b = bdd_storage_alloc();
  bdd_fn_t fn = bdd_val_const(b, 0);

  for(int i = 0; i < size; i++){
    if(truth[i]){
      bdd_fn_t curr = bdd_val_const(b, 1);
      for(int k = 0; k < bits; k++){
        bdd_fn_t v = bdd_val_single(b, k);
        if((i & (1 << k)) == 0){
          v = bdd_op_not(b, v);
        }
        curr = bdd_op_and(b, curr, v);
      }

      fn = bdd_op_or(b, fn, curr);
    }
  }

  bdd_sumCached_t c = bdd_sumCached_new(b, bits, bits);

  // assuming NUMBER_SIZE(bits+3)==1
  number_t transform[size];
  bdd_sumCached_transform(c, fn, transform);
  for(int i = 0; i < size; i++){
    printf("%d ", transform[i]);
  }
  printf("\n");

  bdd_sumCached_delete(c);
  bdd_storage_free(b);
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
  local truth=$1

  echo checking: $truth
  genExpected $truth >$dir/expected
  genActual $truth >$dir/actual

  diff $dir/expected $dir/actual >$dir/diff
  if [ $? -eq 0 ] ; then
    echo "Correct!"
  else
    echo "ERR: The results differ!"
    paste <(echo 'expected:' ; cat $dir/expected) <(echo 'actual:' ; cat $dir/actual)
  fi
  echo
}

test 0,0,0,0
test 1,1,1,1
test 1,0,0,0
test 0,1,0,0
test 1,1,0,0
echo "Done."
