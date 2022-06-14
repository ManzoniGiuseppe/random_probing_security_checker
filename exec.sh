#!/bin/bash

if [ "$#" -ne 2 ] ; then
  echo 'params: <gadget>.sage <maxCoeff>'
  exit
fi

trap 'rm -f "$tmp" "$tmp_c" "$tmp_input"' EXIT
tmp=$(mktemp) || exit 1
tmp_input=$(mktemp) || exit 1
tmp_c=$(mktemp).c || exit 1

# save file in case it's from a pipe
cat $1 > $tmp_input

# get '#' info
d=$(cat $tmp_input | grep '#SHARES' | sed 's/#SHARES //')
numRnd=$(cat $tmp_input | grep '#RANDOMS' | sed 's/#RANDOMS //;s/ /\n/g' | wc -l)
numUndIns=$(cat $tmp_input | grep '#IN' | sed 's/#IN //;s/ /\n/g' | wc -l)
numUndOuts=$(cat $tmp_input | grep '#OUT' | sed 's/#OUT //;s/ /\n/g' | wc -l)
ins=$(cat $tmp_input | grep '#IN' | sed 's/#IN //')
outs=$(cat $tmp_input | grep '#OUT' | sed 's/#OUT //')
randoms=$(cat $tmp_input | grep '#RANDOMS' | sed 's/#RANDOMS //')

# remove comments and empty lines
cat $tmp_input | sed 's/^#.*$//;s/^[^a-z0-9]*$//' | sed "s/^[ \t]*//" | grep -v '^$' > $tmp
cat $tmp > $tmp_input

# get variables assigned to
vars="$(cat $tmp_input | sed 's/ .*$/,/' | sort | uniq | tr '\n' ' ' | sed 's/, $//')"


# translate the compuations
current=$[ numUndOuts * d ]
used=";"
cat $tmp_input | while read l ; do
  # result
  v0=$(echo "$l" | sed 's/ .*$//')
  # input 1
  v1=$(echo "$l" | sed 's/^[^=]*= *//;s/ *[+*].*$//')
  # input 2
  v2=$(echo "$l" | sed 's/^.*[+*] *//')

  # multiplicity for v1
  echo "  ret[${current}] = ${v1};"
  current=$[ current + 1 ]
  if grep -q ";${v1};" <(echo "${used}") ; then
    echo "  ret[${current}] = ${v1};"
    current=$[ current + 1 ]
  else
    used=$(echo "${used}${v1};")
  fi

  # multiplicity for v2
  echo "  ret[${current}] = ${v2};"
  current=$[ current + 1 ]
  if grep -q ";${v2};" <(echo "${used}") ; then
    echo "  ret[${current}] = ${v2};"
    current=$[ current + 1 ]
  else
    used=$(echo "${used}${v2};")
  fi

  # new out means to reset the usage
  used=$(echo "$used" | sed "s/;${v0};/;/" )

  # print the calculation
  echo "  $l;" | tr '+*' '^&'
done > $tmp





# get info
numProb=$(cat $tmp | grep "^  ret\[.*\] = " | wc -l)
numIns=$[ numRnd + numUndIns * d ]
numOuts=$[ numProb + numUndOuts * d ]



# start the c file
echo -n > $tmp_c

# add the needed defines
echo "#define NUM_INS ${numUndIns}" >> $tmp_c
echo "#define NUM_OUTS ${numUndOuts}" >> $tmp_c
echo "#define D ${d}" >> $tmp_c
echo "#define NUM_RANDOMS ${numRnd}" >> $tmp_c
echo "#define NUM_PROBES ${numProb}" >> $tmp_c
echo "#define MAX_COEFF ${2}" >> $tmp_c

# add the c engine
cat main.c >> $tmp_c

# define the gadget function
echo 'void gadget(bool x[NUM_TOT_INS], bool ret[NUM_TOT_OUTS]){' >> $tmp_c

# add inputs variables from the input vector
current=0
for letter in $ins ; do
  for i in $(seq 0 $[d - 1]) ; do
    echo "  bool ${letter}${i} = x[${current}];" >> $tmp_c
    current=$[current + 1]
  done
done

# add random variables from the input vector
echo $randoms | sed 's/ /\n/g' | awk "BEGIN {i = ${current}} ; "'{print "  bool " $1 " = x[" i++ "];" } ' >> $tmp_c

# define variables
echo "  bool $vars;" >> $tmp_c

#place the computations
cat $tmp >> $tmp_c





# save the outputs
current=0
for letter in $outs ; do
  for i in $(seq 0 $[d-1]) ; do
    echo "  ret[${current}] = ${letter}${i};"
    current=$[ current + 1 ]
  done
done >> $tmp_c

# terminate the gadget function
echo '}' >> $tmp_c

# print to allow doublechecking
cat $tmp_c

# compile
gcc -O3 -Wall $tmp_c -o $tmp

# exec
$tmp
