#!/bin/sh
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.


oDir='.o'
srcDir='src'
out='rpsc'



numThreads=1
isDbg=0
isCheck=0

while [ "$#" -gt 0 ]; do
  case "$1" in
    -t) numThreads="$2"; shift 2;;
    --dbg) isDbg=1; shift 1;;
    --check) isCheck=1; shift 1;;

    -*) echo "Unknown option: $1" >&2; exit 1;;
    *) echo "No option specified: $1" >&2; shift 1;;
  esac
done

if [ "$numThreads" -lt 1 ] ; then
  echo "Invalid number of threads ${numThreads}"
  exit 1
fi





mkdir ${oDir} 2>/dev/null
touch ${oDir}/toDelete
rm ${oDir}/*

supportedChar="-_a-zA-Z0-9 :/<>.,:;\`\"'(){}#?+*"
if [ $(cat gpl-3.0.txt | sed "s/[${supportedChar}]//g" | grep -v "^$" | wc -l) -ne 0 ] ; then
  echo "The license was modified, and now it has unsupported characters."
  cat gpl-3.0.txt | sed "s/[${supportedChar}]//g" | grep -v "^$"
  exit 1
fi
echo "char license[] = \\" > ${oDir}/license.c
cat gpl-3.0.txt | sed 's/"/\\"/g;s/^/  "/;s/$/\\n"\\/' >> ${oDir}/license.c
echo ";"  >> ${oDir}/license.c


if [ $(cat README.md | sed "s/[${supportedChar}]//g" | grep -v "^$" | wc -l) -ne 0 ] ; then
  echo "The README.md was modified, and now it has unsupported characters."
  cat README.md | sed "s/[${supportedChar}]//g" | grep -v "^$"
  exit 1
fi
echo "char help[] = \\" > ${oDir}/help.c
cat README.md | sed 's/"/\\"/g;s/^/  "/;s/$/\\n"\\/' >> ${oDir}/help.c
echo ";"  >> ${oDir}/help.c


# MAX_NUM_MASKED_INS must be <= 64

hashCache_splitBits=0
hashCache_splitNum=1
while [ "$hashCache_splitNum" -le $(expr "${numThreads}" \* 8) ] ; do # ensure probability of getting a locked split is ~12 %
  hashCache_splitBits=$(expr ${hashCache_splitBits} + 1)
  hashCache_splitNum=$(expr ${hashCache_splitNum} \* 2)
done

preprocFlags=""
preprocFlags="${preprocFlags} -DNUM_THREADS=$numThreads"
preprocFlags="${preprocFlags} -DMAX_NUM_TOT_INS=126"
preprocFlags="${preprocFlags} -DMEM_NUM_ALLOCS=10000"
preprocFlags="${preprocFlags} -DHASHMAP_INITIAL_BITS=7"
preprocFlags="${preprocFlags} -DHASHMAP_CONTIGUOUS_BITS=3"
preprocFlags="${preprocFlags} -DHASHMAP_HASH_ATTEMPTS=5"
preprocFlags="${preprocFlags} -DHASHMAP_SAVE_HASH_RATIO=5"
preprocFlags="${preprocFlags} -DHASHCACHE_BITS=22"
preprocFlags="${preprocFlags} -DHASHCACHE_WAYS=4"
preprocFlags="${preprocFlags} -DHASHCACHE_SPLIT_BITS=${hashCache_splitBits}"
preprocFlags="${preprocFlags} -DFN_CMP_STEP=0.0001"
preprocFlags="${preprocFlags} -DMAX_LEN_VAR_NAMES=100"
preprocFlags="${preprocFlags} -DMAX_FILE_SIZE=1000000"

if [ "$isDbg" -eq 1 ] ; then
  dbgLvl="DBG_LVL_MAX"
  dbgLvl="DBG_LVL_DETAILED"
  dbgLvl="DBG_LVL_MINIMAL"
  preprocFlags="${preprocFlags} -DDBG_ROWINFO=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_WRAPPER=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_MEM=${dbgLvl}"
  dbgLvl="DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_HASHMAP=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_ROWHASHED=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_HASH=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_BDD=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_HASHCACHE=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_BITARRAY=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_SUBROWHASHED=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_ROWINDEXEDSET=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_TRANSFORMGENERATOR=${dbgLvl}"
  # don't use DBG_LVL_NONE in combination to the others, some dbg information are only calculated if some DBG is present, and may be used by the DBG of other modules.
elif [ "$isCheck" -eq 1 ] ; then
  preprocFlags="${preprocFlags} -DDBG_HASHMAP=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_MEM=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_HASH=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_BDD=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_HASHCACHE=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_BITARRAY=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_WRAPPER=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_ROWHASHED=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_ROWINFO=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_SUBROWHASHED=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_ROWINDEXEDSET=DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_TRANSFORMGENERATOR=DBG_LVL_TOFIX"
else
  preprocFlags="${preprocFlags} -DDBG_HASHMAP=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_MEM=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_HASH=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_BDD=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_HASHCACHE=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_BITARRAY=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_WRAPPER=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_ROWHASHED=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_ROWINFO=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_SUBROWHASHED=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_ROWINDEXEDSET=DBG_LVL_NONE"
  preprocFlags="${preprocFlags} -DDBG_TRANSFORMGENERATOR=DBG_LVL_NONE"
fi

optimizeFlag=""
optimizeFlag="${optimizeFlag} -march=native"
optimizeFlag="${optimizeFlag} -mtune=native"
optimizeFlag="${optimizeFlag} -flto"
optimizeFlag="${optimizeFlag} -O3"
optimizeFlag="${optimizeFlag} -fwhole-program"

warningFlag=""
warningFlag="${warningFlag} -Wall"
warningFlag="${warningFlag} -Wextra"
warningFlag="${warningFlag} -Winline"
warningFlag="${warningFlag} -Werror"

standardFlag="--std=gnu2x"

libFlag=""
if [ $numThreads -gt 1 ] ; then
  libFlag="${libFlag} -lpthread"
fi

for name in $(cd ${srcDir} ; ls *.c | sed 's/.c$//') ; do
  gcc -c $optimizeFlag $warningFlag $standardFlag $preprocFlags "${srcDir}/${name}.c" -o "${oDir}/src_${name}.o" || exit 1
done
for name in $(cd ${oDir} ; ls *.c | sed 's/.c$//') ; do
  gcc -c $optimizeFlag $warningFlag $standardFlag $preprocFlags "${oDir}/${name}.c" -o "${oDir}/o_${name}.o" || exit 1
done

gcc $optimizeFlag $warningFlag $standardFlag ${oDir}/*.o $libFlag -o $out || exit 1

echo "Done."
