#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

mainDir=.rpsc.out
timeout=4h

# 1: in file name
# 2: in options
# 3: out file name
# 4: maxCoeff
# 5: maxAllowedCoeff
function findCoeffAndSaveResults(){
  local file="$1"
  local op="$2"
  local out="$3"
  local maxCoeff="$4"
  local maxAllowedCoeff="$5"

  mkdir $out 2>/dev/null

  # limit
  if [ "$maxCoeff" -ge "$maxAllowedCoeff" ] ; then
    local maxCoeff=$maxAllowedCoeff
  fi

  for c in $(seq 0 $maxCoeff) ; do
    if [ -e $out/$c.timeout ] || [ -e $out/$c.otherfail ] ; then
      return
    fi
    if [ ! -e $out/$c.success ] ; then
      echo "calculating ./rpsc --sage $file $op -c $c > $out"
      { timeout "$timeout" bash -c "time ./rpsc --sage $file $op -c $c" ; } >$mainDir/_tmp_out 2>&1
      local status=$?

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
# 3: maxAllowedCoeff_rps
# 4: maxAllowedCoeff_rpc
function execGadget(){
  local name="$1"
  local file="$2"
  local maxAllowedCoeff_rps="$3"
  local maxAllowedCoeff_rpc="$4"

  mkdir $mainDir/$name 2>/dev/null

  local maxCoeff=$(./rpsc --sage $file --printGadget | grep -a "^NUM_TOT_PROBES=" | sed "s/^NUM_TOT_PROBES=//")
  local d=$(grep -a "^#SHARES " $file | sed "s/^#SHARES //")

  echo $name - ${maxAllowedCoeff_rps} - ${maxAllowedCoeff_rpc} = ${maxCoeff} - ${d}

  for op in $(echo -e "rpsCor3\nrpsCor2\nrpsCor1") ; do
    findCoeffAndSaveResults "$file" "--$op" "$mainDir/$name/$op" "$maxCoeff" "${maxAllowedCoeff_rps}"
  done

  local t=$[d / 2] # ignore the others.
  for op in $(echo -e "rpcCor2\nrpcCor1") ; do
    findCoeffAndSaveResults "$file" "--${op} -t $t" "$mainDir/$name/${op}__$t" "$maxCoeff" "${maxAllowedCoeff_rpc}"
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

gadgets=""
gadgets="${gadgets} vrapsPaper_mul.sage:6:5"
gadgets="${gadgets} vrapsPaper_add_v3.sage:9:5"
gadgets="${gadgets} vrapsPaper_copy.sage:13:1"
gadgets="${gadgets} otpoePaper_small_add.sage:9:5"
gadgets="${gadgets} otpoePaper_small_mul.sage:1:5"
gadgets="${gadgets} otpoePaper_small_refresh.sage:1:6"

for gadgetCoeff in $(echo "$gadgets" | sed "s/^ *//;s/ /\n/") ; do
  gadget=$(echo "$gadgetCoeff" | sed "s/:.*:.*//")
  coeff=$(echo "$gadgetCoeff" | sed "s/^[^:]*://")
  execGadget $gadget gadgets/$gadget $(echo "$coeff" | sed "s/:/ /g")
done

generators=""
generators="${generators} isw_mul.py__3:8:5"
generators="${generators} otpoePaper_mul.py__3:5:1"
generators="${generators} otpoePaper_add.py__3:9:1"
generators="${generators} otpoePaper_copy.py__3:1:6"
generators="${generators} isw_refresh.py__3:1:9"
generators="${generators} otpoePaper_add.py__3:1:5"
generators="${generators} otpoePaper_copy.py__3:1:5"
generators="${generators} otpoePaper_add.py__5:7:3"

for generatorCoeff in $(echo "$generators" | sed "s/^ *//;s/ /\n/") ; do
  genD=$(echo "$generatorCoeff" | sed "s/:.*:.*//")
  coeff=$(echo "$generatorCoeff" | sed "s/^[^:]*://")
  generator=$(echo "$genD" | sed "s/__.*//")
  d=$(echo "$genD" | sed "s/.*__//")

  gadgets/$generator $d > $mainDir/_tmp_input
  execGadget "${generator}__$d" "$mainDir/_tmp_input" $(echo "$coeff" | sed "s/:/ /g")
done


rm $mainDir/_tmp_*
