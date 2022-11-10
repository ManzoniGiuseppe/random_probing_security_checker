#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

mainDir=.rpsc.out
maxAllowedCoeff=5
tiemout=120m

# 1: in file name
# 2: in options
# 3: out file name
# 4: maxCoeff
function findCoeffAndSaveResults(){
  file="$1"
  op="$2"
  out="$3"
  maxCoeff="$4"

  mkdir $out 2>/dev/null

  # limit
  if [ "$maxCoeff" -ge "$maxAllowedCoeff" ] ; then
    maxCoeff=$maxAllowedCoeff
  fi

  for c in $(seq 0 $maxCoeff) ; do
    if [ -e $out/$c.timeout ] || [ -e $out/$c.otherfail ] ; then
      return
    fi
    if [ ! -e $out/$c.success ] ; then
      echo "calculating ./rpsc --sage $file $op -c $c > $out"
      { timeout $timeout bash -c "time ./rpsc --sage $file $op -c $c" ; } >$mainDir/_tmp_out 2>&1
      status=$?

      if [ $status -eq 0 ] ; then
        cat $mainDir/_tmp_out | grep -a '^rp[sc][a-zA-Z0-9]*-ret: ' | sed 's/^[^:]*: *//' > $out/$c.success
        cat $mainDir/_tmp_out | grep -a '^user' | tail -n 1 | sed 's/^user[\t ]*//;s/m/:/' >> $out/$c.success
      elif [ $status -eq 124 ] ; then
        cat $mainDir/_tmp_out > $out/$c.timeout
        return
      else
        cat $mainDir/_tmp_out > $out/$c.otherfail
        return
      fi
    fi
  done
}


# 1: name
# 2: pseudo-file
function execGadget(){
  name="$1"
  file="$2"

  echo $name
  mkdir $mainDir/$name 2>/dev/null

  maxCoeff=$(./rpsc --sage $file --printGadget | grep -a "^NUM_TOT_PROBES=" | sed "s/^NUM_TOT_PROBES=//")
  d=$(grep -a "^#SHARES " $file | sed "s/^#SHARES //")

  for op in $(echo -e "rpsCor3\nrpsCor2\nrpsCor1") ; do
    findCoeffAndSaveResults "$file" "--$op" "$mainDir/$name/$op" "$maxCoeff"
  done

  t=$[d / 2] # ignore the others.
  for op in $(echo -e "rpcCor2\nrpcCor1") ; do
    findCoeffAndSaveResults "$file" "--${op} -t $t" "$mainDir/$name/${op}__$t" "$maxCoeff"
  done
}



echo compiling...
./compile.sh
status=$?
if [ $status -ne 0 ] ; then
  echo "Failed compilation"
  exit 1
fi


echo generating test results and timings...
mkdir $mainDir 2>/dev/null

for gadget in $(cd gadgets ; ls *.sage) ; do
  execGadget $gadget gadgets/$gadget
done

for generator in $(cd gadgets ; ls *.py) ; do
  for d in $(echo -e '3\n5\n7') ; do
    gadgets/$generator $d > $mainDir/_tmp_input
    execGadget "${generator}__$d" "$mainDir/_tmp_input"
  done
done


rm $mainDir/_tmp_*
