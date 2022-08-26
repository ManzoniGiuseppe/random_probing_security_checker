#!/bin/bash


mainDir=test
allowedOp=':rpsIs:rpsSum:rpsTeo:rpcIs:rpcSum:rpcTeo:'


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

  c=$(ls $out | grep '\.success$' | sed 's:[^/]*/[^/]*/[^/]*/::' | sed 's/\.success$//' | sort -n | tail -n 1)

  echo "calculating ./exec -s $file $op -c $c > _tmp_out"
  { timeout 10m bash -c "time ./exec.sh -s $file $op -c $c" ; } >$mainDir/_tmp_out 2>&1
  status=$?

  if [ $status -ne 0 ] ; then
    echo "FAIL: non 0 exit code!"
  else
    diff <(cat $mainDir/_tmp_out | grep '^RP[SC]: coeffs of ' | sed 's/^[^:]*:[^:]*: *//') <(cat $out/$c.success | head -n 1)
    if [ $? -ne 0 ] ; then
      echo "FAIL: different coefficients!"
    fi
  fi
}


# 1: name
# 2: pseudo-file
function execGadget(){
  name="$1"
  file="$2"

  echo $name
  mkdir $mainDir/$name 2>/dev/null

  ./exec.sh -s $file -c 0 --no-compile > $mainDir/_tmp_out
  maxCoeff=$(grep "^GCC FLAGS: " $mainDir/_tmp_out | sed "s/-D/\n/g" | grep "^TOT_MUL_PROBES=" | sed "s/^TOT_MUL_PROBES=//")
  d=$(grep "^GCC FLAGS: " $mainDir/_tmp_out | sed "s/-D/\n/g" | grep "^D=" | sed "s/^D=//")

  for op in $(echo -e "rpsIs\nrpsSum\nrpsTeo") ; do
    if echo $allowedOp | grep -q ":$op:" ; then
      findCoeffAndSaveResults "$file" "--$op" "$mainDir/$name/$op" "$maxCoeff"
    fi
  done

  for t in $(seq 0 $[d - 1]) ; do
    for op in $(echo -e "rpcIs\nrpcSum\nrpcTeo") ; do
      if echo $allowedOp | grep -q ":$op:" ; then
        findCoeffAndSaveResults "$file" "--${op}=$t" "$mainDir/$name/${op}=$t" "$maxCoeff"
      fi
    done
  done
}


for gadget in $(cd gadgets ; ls *.sage) ; do
  execGadget $gadget gadgets/$gadget
done

for generator in $(cd gadgets ; ls *.py) ; do
  for d in $(seq 2 5) ; do
    gadgets/$generator $d > $mainDir/_tmp_input
    execGadget "$generator=$d" "$mainDir/_tmp_input"
  done
done


rm $mainDir/_tmp_*

