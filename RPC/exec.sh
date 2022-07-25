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
trap 'rm -rf "$dir"' EXIT
dir=$(mktemp -d) || exit 1



# save file in case it's from a pipe
cat $1 > $dir/raw_in



#-- get info and input preprocessing


# get info from '#' lines
d=$(cat $dir/raw_in | grep '#SHARES' | sed 's/#SHARES //')
numRnd=$(cat $dir/raw_in | grep '#RANDOMS' | sed 's/#RANDOMS //;s/ /\n/g' | wc -l)
numUndIns=$(cat $dir/raw_in | grep '#IN' | sed 's/#IN //;s/ /\n/g' | wc -l)
numUndOuts=$(cat $dir/raw_in | grep '#OUT' | sed 's/#OUT //;s/ /\n/g' | wc -l)
ins=$(cat $dir/raw_in | grep '#IN' | sed 's/#IN //')
outs=$(cat $dir/raw_in | grep '#OUT' | sed 's/#OUT //')
randoms=$(cat $dir/raw_in | grep '#RANDOMS' | sed 's/#RANDOMS //')

# remove comments, leading and trailing spaces and empty lines
cat $dir/raw_in | sed 's/^#.*$//;s/^[ \t]*//;s/[ \t]*$//' | grep -v '^$' > $dir/uncommented_in

# get info from the input files
vars="$(cat $dir/uncommented_in | sed 's/ .*$/,/' | sort | uniq | tr '\n' ' ' | sed 's/, $//')"
numLines=$(cat $dir/uncommented_in | wc -l)
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
cat $dir/uncommented_in | sed -f <(genStript_substituteInputs) | tac | sed -f <(genScript_markOutputs) | tac | ./replace_internal_variables.py $[numUndOuts * d] > $dir/translated_assignments

#-- create the c file

# add inputs probes from the input vector
echo -n > $dir/translated_inputs
for i in $(seq 0 $[numIns - 1]) ; do
  echo "ret[$[i + inputProbesOffset]] = x[${i}]" >> $dir/translated_inputs
done

multeplicity_array="$(cat $dir/translated_assignments | ./get_probes_multipicity.py $[numUndOuts * d] $numProb | tr '[]' '{}')"

tot_mul_probes=$(echo "${multeplicity_array}" | tr '{},' '  \n'| awk '{s+=$1} END {print s}')
num_nornd_cols=$[ 1 << ( numUndIns * d ) ]
rows_used_bits=$(./getNumRowsUsed.py $numProb $2 $numUndOuts $d $3)
gcc_flags_macro="-DNUM_INS=${numUndIns} -DNUM_OUTS=${numUndOuts} -DD=${d} -DNUM_RANDOMS=${numRnd} -DNUM_PROBES=${numProb} -DT=${3} -DMAX_COEFF=${2} -DROWTRANSFORM_TRANSFORM_BITS=16 -DROWTRANSFORM_ROW_BITS=$[rows_used_bits-1] -DROWTRANSFORM_ASSOC_BITS=${rows_used_bits} -DTOT_MUL_PROBES=${tot_mul_probes} -DBDD_STORAGE_BITS=24 -DBDD_CACHE_BITS=22 -DBDD_CACHE_WAYS=4 -DNUM_TOT_INS=${numIns} -DNUM_TOT_OUTS=${numOuts} -DNUM_NORND_COLS=${num_nornd_cols} -DFN_CMP_STEP=0.0001"

cat > $dir/gadget.c << EOF
#include "gadget.h"

int gadget_probeMulteplicity[NUM_PROBES] = ${multeplicity_array};

void gadget_fn(void *bdd, bdd_t x[NUM_TOT_INS], bdd_t ret[NUM_TOT_OUTS]){
$(cat $dir/translated_inputs $dir/translated_assignments | sed 's/\(.\) = \(.*\) \([*+]\) \(.*\)/\1 = bdd_op_\3(bdd, \2, \4)/;s/+/xor/;s/\*/and/;;s/^/  /;s/$/;/')
}
EOF


#-- finalize

cp *.c *.h $dir

pushd $dir
trap 'popd >/dev/null ; rm -rf "$dir"' EXIT

# print to allow doublechecking
echo $gcc_flags_macro
cat gadget.c

# compile
souces="gadget $(ls *.c | sed 's/.c$//')"
for name in $souces ; do
  gcc -c -O3 -Wall $gcc_flags_macro ${name}.c -o ${name}.o || exit 1
done

gcc -O3 -Wall *.o -o executable

echo "Compiled!"

# exec
./executable
