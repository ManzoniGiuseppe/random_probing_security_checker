#!/bin/bash

gcc -Wall -DD=3 -DBDD_STORAGE_BITS=20 -DNUM_NORND_COLS=64 -DNUM_INS=2 -DNUM_TOT_INS=10 -DNUM_RANDOMS=4 bdd.test_cmp.c bdd.c -o bdd.test_cmp.a.out
gcc -Wall -DD=3 -DBDD_STORAGE_BITS=20 -DNUM_NORND_COLS=64 -DNUM_INS=2 -DNUM_TOT_INS=10 -DNUM_RANDOMS=4 bdd.test_cmp.c bdd_prot1.c -o bdd.test_cmp.b.out


if diff <(./bdd.test_cmp.a.out) <(./bdd.test_cmp.b.out) ; then
  echo "Success!"
fi

rm bdd.test_cmp.a.out
rm bdd.test_cmp.b.out
