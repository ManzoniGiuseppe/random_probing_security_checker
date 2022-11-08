#!/bin/sh
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.


oDir='.o'
srcDir='src'
out='rpsc'


if [ "$1" = "--dbg" ] ; then
  isDbg=1
else
  isDbg=0
fi

if [ "$1" = "--check" ] ; then
  isCheck=1
else
  isCheck=0
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

preprocFlags=""
preprocFlags="${preprocFlags} -DMAX_NUM_TOT_INS=126"
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

if [ "$isDbg" -eq 1 ] ; then
  dbgLvl="DBG_LVL_MAX"
  dbgLvl="DBG_LVL_DETAILED"
  preprocFlags="${preprocFlags} -DDBG_MEM=${dbgLvl}"
  dbgLvl="DBG_LVL_MINIMAL"
  preprocFlags="${preprocFlags} -DDBG_HASHMAP=${dbgLvl}"
  dbgLvl="DBG_LVL_TOFIX"
  preprocFlags="${preprocFlags} -DDBG_WRAPPER=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_ROWHASHED=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_HASH=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_BDD=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_HASHCACHE=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_BITARRAY=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_ROWINFO=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_SUBROWHASHED=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_ROWINDEXEDSET=${dbgLvl}"
  preprocFlags="${preprocFlags} -DDBG_TRANSFORMGENERATOR=${dbgLvl}"
  dbgLvl="DBG_LVL_NONE"
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
libFlag="${libFlag} -lpthread"

for name in $(cd ${srcDir} ; ls *.c | sed 's/.c$//') ; do
  gcc -c $optimizeFlag $warningFlag $standardFlag $preprocFlags "${srcDir}/${name}.c" -o "${oDir}/src_${name}.o" || exit 1
done
for name in $(cd ${oDir} ; ls *.c | sed 's/.c$//') ; do
  gcc -c $optimizeFlag $warningFlag $standardFlag $preprocFlags "${oDir}/${name}.c" -o "${oDir}/o_${name}.o" || exit 1
done

gcc $optimizeFlag $warningFlag $standardFlag ${oDir}/*.o $libFlag -o $out || exit 1

echo "Done."
