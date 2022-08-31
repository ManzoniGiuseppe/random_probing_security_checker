#!/bin/bash


mainDir=test
maxAllowedCoeff=3

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
  if [ $maxCoeff -ge $maxAllowedCoeff ] ; then
    maxCoeff=$maxAllowedCoeff
  fi

  for c in $(seq 0 $maxCoeff) ; do
    if [ -e $out/$c.timeout ] || [ -e $out/$c.otherfail ] ; then
      return
    fi
    if [ ! -e $out/$c.success ] ; then
      echo "calculating ./exec -s $file $op -c $c > $out"
      { timeout 10m bash -c "time ./exec.sh -s $file $op -c $c" ; } >$mainDir/_tmp_out 2>&1
      status=$?

      if [ $status -eq 0 ] ; then
        cat $mainDir/_tmp_out | grep -a '^RP[SC]: coeffs of ' | sed 's/^[^:]*:[^:]*: *//' > $out/$c.success
        cat $mainDir/_tmp_out | grep -a '^user' | tail -n 1 | sed 's/^user[\t ]*//;s/m/:/' >> $out/$c.success
      elif [ $status -eq 124 ] ; then
        touch $out/$c.timeout
        return
      else
        touch $out/$c.otherfail
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

  ./exec.sh -s $file -c 0 --no-compile > $mainDir/_tmp_out
  maxCoeff=$(grep -a "^GCC FLAGS: " $mainDir/_tmp_out | sed "s/-D/\n/g" | grep -a "^TOT_MUL_PROBES=" | sed "s/^TOT_MUL_PROBES=//")
  d=$(grep -a "^GCC FLAGS: " $mainDir/_tmp_out | sed "s/-D/\n/g" | grep -a "^D=" | sed "s/^D=//")

  for op in $(echo -e "rpsIs\nrpsSum\nrpsTeo") ; do
    findCoeffAndSaveResults "$file" "--$op" "$mainDir/$name/$op" "$maxCoeff"
  done

  for t in $(seq 0 $[d - 1]) ; do
    for op in $(echo -e "rpcIs\nrpcSum\nrpcMix\nrpcTeo") ; do
      findCoeffAndSaveResults "$file" "--${op}=$t" "$mainDir/$name/${op}=$t" "$maxCoeff"
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

