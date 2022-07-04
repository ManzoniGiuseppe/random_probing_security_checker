#!/bin/bash

gcc -DD=3 -DBDD_STORAGE_BITS=20 -DNUM_NORND_COLS=64 -DNUM_INS=2 -DNUM_TOT_INS=10 -DNUM_RANDOMS=4 bdd.test.c bdd.c && ./a.out && rm a.out
