#!/bin/bash


#-- header


if [ "$#" -ne 3 ] ; then
  echo 'params: <gadget.sage> <maxCoeff> <t>'
  exit
fi

if [ ! -e "$1" ] ; then
  echo 'params: <gadget.sage> <maxCoeff> <t>'
  echo 'ERR: the file with the gadget must exist.'
  exit
fi


# create temporary files
trap 'rm -f "$tmp" "$tmp_c" "$tmp_input"' EXIT
tmp=$(mktemp) || exit 1
tmp_input=$(mktemp) || exit 1
tmp_c=$(mktemp).c || exit 1

# save file in case it's from a pipe
cat $1 > $tmp_input


#-- get info and input preprocessing


# get info from '#' lines
d=$(cat $tmp_input | grep '#SHARES' | sed 's/#SHARES //')
numRnd=$(cat $tmp_input | grep '#RANDOMS' | sed 's/#RANDOMS //;s/ /\n/g' | wc -l)
numUndIns=$(cat $tmp_input | grep '#IN' | sed 's/#IN //;s/ /\n/g' | wc -l)
numUndOuts=$(cat $tmp_input | grep '#OUT' | sed 's/#OUT //;s/ /\n/g' | wc -l)
ins=$(cat $tmp_input | grep '#IN' | sed 's/#IN //')
outs=$(cat $tmp_input | grep '#OUT' | sed 's/#OUT //')
randoms=$(cat $tmp_input | grep '#RANDOMS' | sed 's/#RANDOMS //')

# remove comments, leading and trailing spaces and empty lines
cat $tmp_input | sed 's/^#.*$//;s/^[ \t]*//;s/[ \t]*$//' | grep -v '^$' > $tmp
cat $tmp > $tmp_input

# get info from the input files
vars="$(cat $tmp_input | sed 's/ .*$/,/' | sort | uniq | tr '\n' ' ' | sed 's/, $//')"
numLines=$(cat $tmp_input | wc -l)
numIns=$[ numUndIns * d + numRnd ]
numProb=$[ numLines - numUndOuts * d + numIns ]
numOuts=$[ numUndOuts * d + numProb ]
inputProbesOffset=$[ numOuts - numIns ]


#-- preliminary translation, from variable names to 'ret[index]' i.e. directly use the array of output probes and outputs to store things

# it outputs a sed script to substitute the inputs in the code
genStript_substituteInputs(){
  numIn=$inputProbesOffset # first of the probes on the input
  for letter in $ins ; do
    for i in $(seq 0 $[d - 1]) ; do
      echo "s/\b${letter}${i}\b/ret[${numIn}]/g"  # substitute every occurrence with its probe
      numIn=$[numIn + 1]
    done
  done
  for letters in $randoms ; do
    echo "s/\b${letters}\b/ret[${numIn}]/g"  # substitute every occurrence with its probe
    numIn=$[numIn + 1]
  done
}

# it outputs a sed script to mark the outputs in the code
genScript_markOutputs(){
  for letter in $outs ; do
    for i in $(seq 0 $[d - 1]) ; do
      echo "0,/^${letter}${i}\b/s//!${letter}${i}/" # it adds a '!' before the first assignment to any output with the actual output (the input is reversed by tac, so the ! goes to the last assignment to the output)
    done
  done
}

# translate
cat $tmp_input | sed -f <(genStript_substituteInputs) | tac | sed -f <(genScript_markOutputs) | tac | ./replace_internal_variables.py $[numUndOuts * d] > $tmp
cat $tmp > $tmp_input

#-- create the c file

# add inputs probes from the input vector
echo -n > $tmp
for i in $(seq 0 $[numIns - 1]) ; do
  echo "ret[$[i + inputProbesOffset]] = x[${i}]" >> $tmp
done

multeplicity_array="$(cat $tmp_input | ./get_probes_multipicity.py $[numUndOuts * d] $numProb | tr '[]' '{}')"

tot_mul_probes=$(echo "${multeplicity_array}" | tr '{},' '  \n'| awk '{s+=$1} END {print s}')
num_nornd_cols=$[ 1 << ( numUndIns * d ) ]
rows_used_bits=$(./getNumRowsUsed.py $numProb $2 $numUndOuts $d $3)
gcc_flags_macro="-DNUM_INS=${numUndIns} -DNUM_OUTS=${numUndOuts} -DD=${d} -DNUM_RANDOMS=${numRnd} -DNUM_PROBES=${numProb} -DT=${3} -DMAX_COEFF=${2} -DCORRELATIONTABLE_STORAGE_BITS=18 -DROWS_USED_BITS=${rows_used_bits} -DTOT_MUL_PROBES=${tot_mul_probes} -DBDD_STORAGE_BITS=24 -DBDD_CACHE_BITS=15 -DBDD_CACHE_WAYS=4 -DNUM_TOT_INS=${numIns} -DNUM_TOT_OUTS=${numOuts} -DNUM_NORND_COLS=${num_nornd_cols} -DFN_CMP_STEP=0.0001"

cat > $tmp_c << EOF
#include "$(pwd)/gadget.h"

int gadget_probeMulteplicity[NUM_PROBES] = ${multeplicity_array};

void gadget_fn(bdd_t x[NUM_TOT_INS], bdd_t ret[NUM_TOT_OUTS]){
$(cat $tmp $tmp_input | sed 's/\(.\) = \(.*\) \([*+]\) \(.*\)/\1 = bdd_op_\3(\2, \4)/;s/+/xor/;s/\*/and/;;s/^/  /;s/$/;/')
}
EOF


#-- finalize


# print to allow doublechecking
echo $gcc_flags_macro
cat $tmp_c

# compile
gcc -O3 -Wall $gcc_flags_macro $tmp_c main.c row.c correlationTable.c bdd.c probeComb.c -o $tmp

echo "Compiled!"

# exec
$tmp
