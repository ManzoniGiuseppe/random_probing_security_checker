#!/bin/bash

trap 'rm -rf "$dir"' EXIT
dir=$(mktemp -d) || exit 1


function test(){
  logBits=$1
  bits=$2

  echo "Array of ${bits} bits implemented with values of $[1<<${logBits}] bits"

cat > $dir/test.c << EOF
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint$[1 << logBits]_t bitArray_i_value_t;
#define BITARRAY_I_VALUE_LOG_BITS ${logBits}
#include "$(pwd)/../src/bitArray.h"

int bits = ${bits};

#define ASSERT(actual, expected, format, ...) {\
  int a = (actual);\
  int e = (expected);\
  if(a != e) FAIL("Obtained value %d instead of %d. " format "\n", a, e, __VA_ARGS__)\
}

int main(){
  BITARRAY_DEF_VAR(bits, v1)
  bitArray_zero(bits, v1);

  printf("Check 0\n");
  for(int i = 0; i < bits; i++)
    ASSERT(bitArray_get(v1, i), 0, "%s", "zero or get fails")
  ASSERT(bitArray_count1(bits, v1), 0, "%s", "count fails.")

  printf("Check single bit\n");
  for(int pos = 0; pos < bits; pos++){
    bitArray_set(v1, pos);

    for(int i = 0; i < bits; i++)
      ASSERT(bitArray_get(v1, i), (i == pos), "zero or get fails with bit in pos %d", pos)
    ASSERT(bitArray_count1(bits, v1), 1, "count fails with bit in pos %d", pos)
    ASSERT(bitArray_tail1(bits, v1), pos, "tail fails with bit in pos %d", pos)
    ASSERT(bitArray_lead1(bits, v1), pos, "lead fails with bit in pos %d", pos)

    bitArray_reset(v1, pos);

    for(int i = 0; i < bits; i++)
      ASSERT(bitArray_get(v1, i), 0, "reset or get fails with bit in pos %d", pos)
  }

  printf("Check with fixed bit\n");
  bitArray_set(v1, bits/2);
  for(int pos = 0; pos < bits; pos++){
    if(pos == bits/2) continue; // already set

    bitArray_set(v1, pos);

    for(int i = 0; i < bits; i++)
      ASSERT(bitArray_get(v1, i), (i == pos || i == bits/2), "zero or get fails with bit in pos %d", pos)
    ASSERT(bitArray_count1(bits, v1), 2, "count fails with bit in pos %d", pos)
    ASSERT(bitArray_tail1(bits, v1), (pos > bits/2 ? bits/2 : pos), "tail fails with bit in pos %d", pos)
    ASSERT(bitArray_lead1(bits, v1), (pos > bits/2 ? pos : bits/2), "lead fails with bit in pos %d", pos)

    bitArray_reset(v1, pos);

    for(int i = 0; i < bits; i++)
      ASSERT(bitArray_get(v1, i), (i == bits/2), "reset or get fails with bit in pos %d", pos)
  }
  bitArray_reset(v1, bits/2);

  printf("Check lowest\n");
  ASSERT(bitArray_lowest_get(v1, 4), 0, "%s", "lowest 4 bits\n")
  bitArray_set(v1, 3);
  bitArray_set(v1, 4);
  ASSERT(bitArray_lowest_get(v1, 5), 8+16, "%s", "lowest 4 bit\n")
  ASSERT(bitArray_lowest_get(v1, 4), 8, "%s", "lowest 4 bit\n")
  ASSERT(bitArray_lowest_get(v1, 3), 0, "%s", "lowest 3 bit\n")
  bitArray_lowest_set(v1, 3, 0);
  ASSERT(bitArray_lowest_get(v1, 4), 8, "%s", "lowest 4 bit after set\n")
  bitArray_lowest_set(v1, 4, 0);
  ASSERT(bitArray_lowest_get(v1, 4), 0, "%s", "lowest 4 bit after set\n")
  ASSERT(bitArray_lowest_get(v1, 5), 16, "%s", "lowest 5 bit after set\n")
  bitArray_lowest_set(v1, 5, 0);
  ASSERT(bitArray_lowest_get(v1, 5), 0, "%s", "lowest 5 bit after set\n")

  printf("Check eq\n");
  BITARRAY_DEF_VAR(bits, v2)
  bitArray_zero(bits, v2);

  ASSERT(bitArray_eq(bits, v1, v2), 1, "%s", "0 shuould be == 0")

  bitArray_set(v1, bits-1);
  ASSERT(bitArray_eq(bits, v1, v2), 0, "%s", "1<<(bits-1) shuould be != 0")

  bitArray_set(v2, bits-1);
  ASSERT(bitArray_eq(bits, v1, v2), 1, "%s", "1<<(bits-1) shuould be == 1<<(bits-1)")

  bitArray_reset(v1, bits-1);
  bitArray_set(v1, 0);
  ASSERT(bitArray_eq(bits, v1, v2), 0, "%s", "1 shuould be != 1<<(bits-1)")

  bitArray_reset(v1, 0);
  bitArray_reset(v2, bits-1);

  printf("Check not\n");
  bitArray_not(bits, v1, v2);
  ASSERT(bitArray_count1(bits, v2), bits, "%s", "not or count fails.")

  bitArray_reset(v2, bits/2);
  bitArray_not(bits, v2, v1);
  ASSERT(bitArray_count1(bits, v1), 1, "%s", "not or count fails.")
  ASSERT(bitArray_get(v1, bits/2), 1, "%s", "not or get fails.")
  bitArray_zero(bits, v1);
  bitArray_zero(bits, v2);


  printf("Check ops\n");
  BITARRAY_DEF_VAR(bits, ret)

  bitArray_set(v1, bits-1);
  bitArray_set(v1, bits-2);
  bitArray_set(v1, 0);
  ASSERT(bitArray_count1(bits, v1), 3, "%s", "set or count fails.")

  bitArray_set(v2, bits-2);
  bitArray_set(v2, bits/2);
  bitArray_lowest_set(v2, 2, 3);
  ASSERT(bitArray_count1(bits, v2), 4, "%s", "set or count fails.")


  bitArray_or(bits, v1, v2, ret);
  ASSERT(bitArray_count1(bits, ret), 5, "%s", "or or count fails.")
  bitArray_and(bits, v1, v2, ret);
  ASSERT(bitArray_count1(bits, ret), 2, "%s", "and or count fails.")
  bitArray_xor(bits, v1, v2, ret);
  ASSERT(bitArray_count1(bits, ret), 3, "%s", "xor or count fails.")


  printf("TODO: bitArray_localInc bitArray_hash");


  printf("Success!\n");
  return 0;
}

EOF

  gcc -O3 -Wall -Wextra $dir/test.c -o $dir/test
  if [ "$?" -eq 0 ] ;then
    $dir/test
  else
     echo Compilation failed!
  fi

  echo
}

test 3 24
test 3 22
test 4 24

echo TODO: check assertions really fail
echo Done
