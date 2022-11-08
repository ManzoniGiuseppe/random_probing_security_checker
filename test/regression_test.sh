#!/bin/bash
#This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
#Copyright (C) 2022  Giuseppe Manzoni
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.


allowedOp=':rpsCor1:rpsCor2:rpsCor3:rpcCor1:rpcCor2:'
maxC=1



trap 'rm -rf "$tmpOut" "$tmpIn"' EXIT
tmpOut=$(mktemp) || exit 1
tmpIn=$(mktemp) || exit 1


# 1: in file name
# 2: in options
# 3: out file name
function findCoeffAndCheckResults(){
  file="$1"
  op="$2"
  out="$3"

  c=$(cat .regression.test | grep -a "^${out}/[0-9]*\.success" | sed 's|^[^/]*/[^/]*/\([0-9]*\)\.success: .*$|\1|' | sort -n | tail -n 1)

  if [ -z "$c" ] ; then
    echo missing $out
    return
  fi

  if [ "$c" -ge "$maxC" ] ; then
    c="$maxC"
  fi

  echo "calculating ./rpsc --sage $file $op -c $c > _tmp_out"
  { timeout 10m bash -c "time ../rpsc --sage $file $op -c $c" ; } >${tmpOut} 2>&1
  status=$?

  if [ $status -ne 0 ] ; then
    echo "FAIL: non 0 exit code!" 2>&1
    cat ${tmpOut}
    exit 1
  else
    cat ${tmpOut} | tail -n 3 | grep "^user"
    diff <(cat ${tmpOut} | grep -a '^rp[sc][a-zA-Z0-9]*-ret: ' | sed 's/^[^:]*: *//') <(cat .regression.test | grep -a "^${out}/${c}\.success: " | sed "s|${out}/${c}\.success: ||")
    if [ $? -ne 0 ] ; then
      echo "FAIL: different coefficients!" 2>&1
      cat ${tmpOut}
      exit 1
    fi
  fi
}


# 1: name
# 2: pseudo-file
function execGadget(){
  name="$1"
  file="$2"

  echo $name

  d=$(grep -a "^#SHARES " $file | sed "s/^#SHARES //")

  for op in $(echo -e "rpsCor3\nrpsCor2\nrpsCor1") ; do
    if echo $allowedOp | grep -a -q ":$op:" ; then
      findCoeffAndCheckResults "$file" "--$op" "$name/$op"
    fi
  done

  for t in $(seq 0 $[d - 1]) ; do
    for op in $(echo -e "rpcCor2\nrpcCor1") ; do
      if echo $allowedOp | grep -a -q ":${op}:" ; then
        findCoeffAndCheckResults "$file" "--${op} -t $t" "$name/${op}__$t"
      fi
    done
  done
}



if [ ! -f ".regression.test" ] ; then
  echo "no file with regression tests: nothing to do!"
  exit 0
fi


echo compiling...

pushd .. >/dev/null
./compile.sh --check
status=$?
popd >/dev/null

if [ $status -ne 0 ] ; then
  echo "Failed compilation"
  exit 1
fi

echo testing...
#for gadget in $(cd ../gadgets ; ls *.sage) ; do
#  execGadget $gadget ../gadgets/$gadget
#done

for generator in $(cd ../gadgets ; ls *.py) ; do
  for d in $(seq 2 5) ; do
    ../gadgets/$generator $d > $tmpIn
    execGadget "${generator}__$d" "$tmpIn"
  done
done

