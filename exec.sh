#!/bin/bash


#-- header

operations=""
paramT=0  # for rps
nocompile=0 # to get the info without wasting time

while [ "$#" -gt 0 ]; do
  case "$1" in
    -s) paramSage="$2"; shift 2;;
    -c) paramMaxCoeff="$2"; shift 2;;

    --rps) operations="${operations} -DWITH_RPS_IS -DWITH_RPS_SUM -DWITH_RPS_TEO"; shift 1;;
    --rpsIs) operations="${operations} -DWITH_RPS_IS"; shift 1;;
    --rpsSum) operations="${operations} -DWITH_RPS_SUM"; shift 1;;
    --rpsTeo) operations="${operations} -DWITH_RPS_TEO"; shift 1;;
    --rpc=*) operations="${operations} -DWITH_RPC_IS -DWITH_RPC_W -DWITH_RPC_TEO"; paramT="${1#*=}"; shift 1;;
    --rpcIs=*) operations="${operations} -DWITH_RPC_IS"; paramT="${1#*=}"; shift 1;;
    --rpcW=*) operations="${operations} -DWITH_RPC_W"; paramT="${1#*=}"; shift 1;;
    --rpcTeo=*) operations="${operations} -DWITH_RPC_TEO"; paramT="${1#*=}"; shift 1;;

    --no-compile) nocompile=1; shift 1;;

    -s=*|--sage=*) paramSage="${1#*=}"; shift 1;;
    -c=*|--maxCoeff=*|--coeff=*) paramMaxCoeff="${1#*=}"; shift 1;;
    --sage|--maxCoeff|--rpcIs|--rpcSum|--rpcTeo) echo "$1 requires an argument" >&2; exit 1;;

    -*) echo "Unknown option: $1" >&2; exit 1;;
    *) echo "No option specified: $1" >&2; shift 1;;
  esac
done

if [ ! -e "$paramSage" ] ; then
  echo "The file .sage with the gadget must exist. Not: $paramSage" >&2
  exit 1
fi

if [ -z "$operations" ] ; then
  echo 'No operation specified!'
fi


# create temporary files
trap 'rm -rf "$dir"' EXIT
dir=$(mktemp -d) || exit 1



# save file in case it's from a pipe
cat "$paramSage" > $dir/raw_in



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
cat $dir/uncommented_in | sed -f <(genStript_substituteInputs) | tac | sed -f <(genScript_markOutputs) | tac | ./exec_replaceInternalVariables.py $[numUndOuts * d] > $dir/translated_assignments

#-- create the c file

# add inputs probes from the input vector
echo -n > $dir/translated_inputs
for i in $(seq 0 $[numIns - 1]) ; do
  echo "ret[$[i + inputProbesOffset]] = x[${i}]" >> $dir/translated_inputs
done

multeplicity_array="$(cat $dir/translated_assignments | ./exec_getProbesMultiplicity.py $[numUndOuts * d] $numProb | tr '[]' '{}')"

tot_mul_probes=$(echo "${multeplicity_array}" | tr '{},' '  \n'| awk '{s+=$1} END {print s}')

if [ -z "$paramMaxCoeff" ] ; then
  paramMaxCoeff=$tot_mul_probes
elif [ $tot_mul_probes -lt $paramMaxCoeff ] ; then
  paramMaxCoeff=$tot_mul_probes
fi

num_nornd_cols=$[ 1 << ( numUndIns * d ) ]
rows_used_bits=$(./exec_getNumRowsUsed.py $numProb $paramMaxCoeff $numUndOuts $d $paramT)
ii_used_comb=$(./exec_getNumIIUsed.py $numUndIns $d $paramT)
if [ $rows_used_bits -ge 31 ] ; then
  echo "Too much ram would need to be allocated! 2^$[rows_used_bits + numUndIns * d * 2 + 2 ] B " >&2
  exit 1
fi
gcc_flags_macro="${operations} -DNUM_INS=${numUndIns} -DNUM_OUTS=${numUndOuts} -DD=${d} -DNUM_RANDOMS=${numRnd} -DNUM_PROBES=${numProb} -DT=${paramT} -DMAX_COEFF=${paramMaxCoeff} -DROWTRANSFORM_UNIQUE_BITS=${rows_used_bits} -DTOT_MUL_PROBES=${tot_mul_probes} -DRPCTEO_HTPROBE_BITS=12 -DBDD_STORAGE_BITS=24 -DBDD_CACHE_BITS=24 -DBDD_CACHE_WAYS=4 -DNUM_TOT_INS=${numIns} -DNUM_TOT_OUTS=${numOuts} -DNUM_NORND_COLS=${num_nornd_cols} -DII_USED_COMB=${ii_used_comb} -DFN_CMP_STEP=0.0001"

cat > $dir/gadget.c << EOF
#include "gadget.h"

int gadget_probeMulteplicity[NUM_PROBES] = ${multeplicity_array};

void gadget_fn(void *bdd, bdd_t x[NUM_TOT_INS], bdd_t ret[NUM_TOT_OUTS]){
$(cat $dir/translated_inputs $dir/translated_assignments | sed 's/\(.\) = \(.*\) \([*+]\) \(.*\)/\1 = bdd_op_\3(bdd, \2, \4)/;s/+/xor/;s/\*/and/;;s/^/  /;s/$/;/')
}
EOF


#-- finalize

cp src/* $dir

pushd $dir
trap 'popd >/dev/null ; rm -rf "$dir"' EXIT

# print to allow doublechecking
echo "GCC FLAGS: " $gcc_flags_macro
cat gadget.c

# compile

if [ $nocompile -eq 1 ] ; then exit ; fi

souces="gadget $(ls *.c | sed 's/.c$//')"
for name in $souces ; do
  gcc -c -O3 -flto -march=native -mtune=native -Wall -Wextra $gcc_flags_macro ${name}.c -o ${name}.o || exit 1
done

gcc -O3 -flto -march=native -mtune=native -fwhole-program -Wall -Wextra *.o -o executable

echo "Compiled!"

# exec
./executable
