#!/bin/bash

gcc -DD=3 -DNUM_INS=1 -DNUM_OUTS=1 -DNUM_PROBES=3 -DNUM_TOT_OUTS=6 -DT=2 -DNUM_NORND_COLS=8 -DMAX_COEFF=2 row_prot1.test.c row_prot1.c && ./a.out && rm a.out
