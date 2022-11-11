#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

trap 'rm -rf "$dir"' EXIT
dir=$(mktemp -d) || exit 1

genExpected(){
  local gadget=$1

  local d=$(cat $gadget | grep "^#SHARES " | sed "s/^#SHARES //")
  local is=$(cat $gadget | grep "^#IN " | sed "s/^#IN //")
  local os=$(cat $gadget | grep "^#OUT " | sed "s/^#OUT //")
  local rs=$(cat $gadget | grep "^#RANDOMS " | sed "s/^#RANDOMS //")

  local ni=0
  for i in $is ; do ni=$[ni+1] ; done
  local no=0
  for o in $os ; do no=$[no+1] ; done
  local nr=0
  for r in $rs ; do nr=$[nr+1] ; done


cat > $dir/printOutputTransforms.py << EOF
#!/usr/bin/env python3

from sympy import fwht

def gadget(inArr):
$(
  local c=0;
  for i in $is ; do
    for k in $(seq 0 $[d-1]) ; do
      echo "  ${i}${k} = inArr[$c]"
      local c=$[c+1]
    done
  done
  for r in $rs ; do
    echo "  ${r} = inArr[$c]"
    local c=$[c+1]
  done
)

$(cat $gadget | sed "s/^/  /;s/+/^/;s/*/&/")

  outArr=[]
$(
  for o in $os ; do
    for k in $(seq 0 $[d-1]) ; do
      echo "  outArr+=[${o}${k}]"
    done
  done
)

  return outArr


def int2bitarr(bits, i):
  return [(i >> b) & 1 for b in range(bits)]

# obtain the truth tables of each output
outs=[$(for i in $(seq 0 $[d*no + d*ni+nr -1-1]) ; do echo -n "[], "; done) []]
for inComb in range(2**$[d*ni+nr]):
  inArr = int2bitarr($[d*ni+nr], inComb)
  outArr = gadget(inArr) + inArr
  for i in range(len(outArr)):
    outs[i] += [outArr[i]]


for o in range(2**len(outs)):
  #which combination of outputs
  use = int2bitarr(len(outs), o)

  #xor the selected outputs
  comb = [0 for o in outs[0]]
  for i in range(len(use)):
    if use[i]:
      comb = [a ^ b for (a,b) in zip(comb, outs[i])]

  #walsh transform
  walsh=fwht([(-1 if c == 1 else 1) for c in comb])[:2**$[d*ni]]

  #print
  for b in walsh:
    print(b, end=" ")
  print("")

EOF

  chmod +x $dir/printOutputTransforms.py
  $dir/printOutputTransforms.py
}

genActual(){
  local gadget=$1

cat > $dir/main.c << EOF
#include "parserSage.h"
#include "rowHashed.h"
#include "transformGenerator.h"

#define NUM_TOT_OUTS  100

typedef struct{
  BITARRAY_DEF_VAR(NUM_TOT_OUTS, wires)
} tryNext_t;

static bool tryNext(void *info, bitArray_t next){
  tryNext_t *s = info;
  return row_tryNext_subrow(NUM_TOT_OUTS, s->wires, next);
}

int main(){
  gadget_t *g;
  {
    gadget_raw_t r = parserSage_parse("${gadget}");
    g = gadget_fromRaw(r);
    r.free(r.fn_info);
  }

  tryNext_t tryNext_info;
  bitArray_zero(NUM_TOT_OUTS, tryNext_info.wires);
  for(wire_t i = 0; i < g->d * g->numOuts + g->numTotIns; i++){
    bitArray_set(NUM_TOT_OUTS, tryNext_info.wires, i);
  }
  BITARRAY_DEF_VAR(NUM_TOT_OUTS, first)
  row_first(NUM_TOT_OUTS, first);
  rowHashed_t rows = rowHashed_alloc(&tryNext_info, NUM_TOT_OUTS, first, tryNext);
  transformGenerator_t tg = transformGenerator_alloc(rows, g);

  wire_t numMaskedIns = g->d * g->numIns;
  fixed_cell_t transform[1ll << numMaskedIns];

  BITARRAY_DEF_VAR(NUM_TOT_OUTS, it)
  row_first(NUM_TOT_OUTS, it);
  do{
    rowHash_t index = rowHashed_hash(rows, it);
    transformGenerator_getTranform(tg, index, numMaskedIns, g->numRnds, transform);
    for(size_t i = 0; i < (1ull << numMaskedIns); i++)
      printf("%d ", transform[i]);
    printf("\n");
  }while(tryNext(&tryNext_info, it));

  rowHashed_free(rows);
  transformGenerator_free(tg);
  gadget_free(g);
  return 0;
}
EOF

preprocFlags=""
preprocFlags="${preprocFlags} -DMAX_NUM_TOT_INS=65"
preprocFlags="${preprocFlags} -DMEM_NUM_ALLOCS=10000"
preprocFlags="${preprocFlags} -DHASHMAP_INITIAL_BITS=7"
preprocFlags="${preprocFlags} -DHASHMAP_CONTIGUOUS_BITS=3"
preprocFlags="${preprocFlags} -DHASHMAP_HASH_ATTEMPTS=5"
preprocFlags="${preprocFlags} -DHASHMAP_SAVE_HASH_RATIO=5"
preprocFlags="${preprocFlags} -DHASHCACHE_BITS=22"
preprocFlags="${preprocFlags} -DHASHCACHE_WAYS=4"
preprocFlags="${preprocFlags} -DFN_CMP_STEP=0.0001"
preprocFlags="${preprocFlags} -DMAX_LEN_VAR_NAMES=100"
preprocFlags="${preprocFlags} -DMAX_FILE_SIZE=1000000"

preprocFlags="${preprocFlags} -DDBG_HASHMAP=1"
preprocFlags="${preprocFlags} -DDBG_HASHCACHE=1"
preprocFlags="${preprocFlags} -DDBG_MEM=1"
preprocFlags="${preprocFlags} -DDBG_BDD=1"
preprocFlags="${preprocFlags} -DDBG_BITARRAY=1"
preprocFlags="${preprocFlags} -DDBG_ROWHASHED=1"
preprocFlags="${preprocFlags} -DDBG_TRANSFORMGENERATOR=1"

sources=""
sources="${sources} $dir/main.c"
sources="${sources} ../src/hashMap.c"
sources="${sources} ../src/parserSage.c"
sources="${sources} ../src/rowHashed.c"
sources="${sources} ../src/transformGenerator.c"
sources="${sources} ../src/bdd.c"
sources="${sources} ../src/types.c"
sources="${sources} ../src/mem.c"
sources="${sources} ../src/hash.c"
sources="${sources} ../src/row.c"
sources="${sources} ../src/gadget.c"
sources="${sources} ../src/hashCache.c"

  gcc -O3 -Wall -Wextra -Werror -I ../src/ $preprocFlags $sources -lpthread -o $dir/prog
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
  local gadget=$1
  local file=$2

  local transformExp=".transform.expected.${gadget}"
  local transformAct="${dir}/transform.actual.${gadget}"

  echo checking $gadget
  cat $file | sed "s/^/  /"

  echo "Generating reference results"
  if ! [ -e $transformExp ] ; then
    genExpected $file > $transformExp
  fi
  echo "Generating actual results"
  genActual $file > $transformAct

  diff $transformExp $transformAct >$dir/diff
  if [ $? -eq 0 ] ; then
    echo "Correct!"
  else
    echo "ERR: The results differ!"
    less $dir/diff
    paste $transformExp $transformAct  | less
  fi
  echo
}

test vrapsPaper_copy ../gadgets/vrapsPaper_copy.sage
test vrapsPaper_add_v1 ../gadgets/vrapsPaper_add_v1.sage
test vrapsPaper_refresh ../gadgets/vrapsPaper_refresh.sage
../gadgets/isw_mul.py 3 > $dir/isw_mul__3.sage
test isw_mul__3.sage $dir/isw_mul__3.sage
echo "Done."
