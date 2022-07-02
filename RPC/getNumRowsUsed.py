#!/usr/bin/env python3

import sys
import re
import math

if len(sys.argv) != 6:
  print("params: <num probes> <max coeff> <num underlying outs> <d> <t>")
  sys.exit(1)



numProbes = int(sys.argv[1])
maxCoeff = int(sys.argv[2])
numOuts = int(sys.argv[3])
d = int(sys.argv[4])
t = int(sys.argv[5])

probeComb = sum(math.comb(numProbes, it) for it in range(maxCoeff+1))
outComb = sum(math.comb(d, it) for it in range(t+1))**numOuts

needed = probeComb * outComb
size = needed * 1.2

size_log2 = math.log(size) / math.log(2)
print(math.ceil(size_log2))
