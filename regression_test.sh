#!/bin/bash


mainDir=test
allowedOp=':rpsIs:rpsSum:rpsTeo:rpcIs:rpcW:rpcTeo:'
maxC=4

# 1: in file name
# 2: in options
# 3: out file name
function findCoeffAndCheckResults(){
  file="$1"
  op="$2"
  out="$3"

  if [ ! -d "$out" ] ; then
    echo missing $out
    return
  fi

  c=$(ls "$out" | grep -a '\.success$' | sed 's:[^/]*/[^/]*/[^/]*/::' | sed 's/\.success$//' | sort -n | tail -n 1)

  if [ -z "$c" ] ; then
    return
  fi

  if [ "$c" -ge "$maxC" ] ; then
    c="$maxC"
  fi

  echo "calculating ./exec -s $file $op -c $c > _tmp_out"
  { timeout 10m bash -c "time ./exec.sh -s $file $op -c $c" ; } >$mainDir/_tmp_out 2>&1
  status=$?

  if [ $status -ne 0 ] ; then
    echo "FAIL: non 0 exit code!"
    cat $mainDir/_tmp_out
  else
    diff <(cat $mainDir/_tmp_out | grep -a '^RP[SC]: coeffs of ' | sed 's/^[^:]*:[^:]*: *//') <(cat $out/$c.success | head -n 1)
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
  if [ ! -d "$mainDir/$name" ] ; then
    echo missing "$mainDir/$name"
    return
  fi

  ./exec.sh -s $file -c 0 --no-compile > $mainDir/_tmp_out
  d=$(grep -a "^GCC FLAGS: " $mainDir/_tmp_out | sed "s/-D/\n/g" | grep -a "^D=" | sed "s/^D=//")

  for op in $(echo -e "rpsIs\nrpsSum\nrpsTeo") ; do
    if echo $allowedOp | grep -a -q ":$op:" ; then
      findCoeffAndCheckResults "$file" "--$op" "$mainDir/$name/$op"
    fi
  done

  for t in $(seq 0 $[d - 1]) ; do
    for op in $(echo -e "rpcIs\nrpcW\nrpcTeo") ; do
      if echo $allowedOp | grep -a -q ":$op:" ; then
        findCoeffAndCheckResults "$file" "--${op}=$t" "$mainDir/$name/${op}__$t"
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
    execGadget "${generator}__$d" "$mainDir/_tmp_input"
  done
done


rm $mainDir/_tmp_*

